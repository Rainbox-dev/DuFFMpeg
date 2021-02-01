﻿#include "Renderer/renderqueue.h"

RenderQueue::RenderQueue(AfterEffects *afterEffects, QObject *parent ) : QObject(parent)
{
    setStatus( MediaUtils::Initializing );

    // === FFmpeg ===

    // The transcoder
    // Create the renderer
    _ffmpegRenderer = new FFmpegRenderer( FFmpeg::instance()->binary() );
    // Connections
    connect( FFmpeg::instance(), &FFmpeg::binaryChanged, _ffmpegRenderer, &FFmpegRenderer::setBinary ) ;
    connect( _ffmpegRenderer, &FFmpegRenderer::newLog, this, &RenderQueue::ffmpegLog ) ;
    connect( _ffmpegRenderer, &FFmpegRenderer::console, this, &RenderQueue::ffmpegConsole ) ;
    connect( _ffmpegRenderer, &FFmpegRenderer::statusChanged, this, &RenderQueue::ffmpegStatusChanged ) ;
    connect( _ffmpegRenderer, &FFmpegRenderer::progress, this, &RenderQueue::ffmpegProgress ) ;
    _ffmpegRenderer->setStopCommand("q\n");

    // === After Effects ===

    // The renderer
    _ae = afterEffects;
    // Create the renderer
    _aeRenderer = new AERenderer( _ae->binary() );
    // Connections
    connect( _ae, &AfterEffects::binaryChanged, _aeRenderer, &AERenderer::setBinary ) ;
    connect( _aeRenderer, &AERenderer::newLog, this, &RenderQueue::aeLog ) ;
    connect( _aeRenderer, &AERenderer::console, this, &RenderQueue::aeConsole ) ;
    connect( _aeRenderer, &AERenderer::statusChanged, this, &RenderQueue::aeStatusChanged ) ;
    connect( _aeRenderer, &AERenderer::progress, this, &RenderQueue::aeProgress ) ;

    // A timer to keep track of the rendering process
    timer = new QTimer( this );
    timer->setSingleShot(true);

    setStatus( MediaUtils::Waiting );
}

RenderQueue::~RenderQueue()
{
    stop(100);
    postRenderCleanUp();
}

void RenderQueue::setStatus(MediaUtils::RenderStatus st)
{
    if( st == _status) return;
    _status = st;
    emit statusChanged(_status);
}

void RenderQueue::ffmpegLog(QString message, LogUtils::LogType lt)
{
    message = "FFmpeg | " + message;
    emit newLog( message, lt );
}

void RenderQueue::ffmpegStatusChanged( MediaUtils::RenderStatus status )
{
    if ( MediaUtils::isBusy( status ) )
    {
        setStatus( MediaUtils::FFmpegEncoding );
        emit newLog("FFmpeg is running.");
    }
    else if ( status == MediaUtils::Finished )
    {
        emit newLog("FFmpeg Transcoding process successfully finished.");
        postRenderCleanUp( MediaUtils::Finished );
    }
    else if ( status == MediaUtils::Stopped )
    {
        emit newLog("FFmpeg transcoding has been stopped.");
        postRenderCleanUp( MediaUtils::Stopped );
    }
    else if ( status == MediaUtils::Error )
    {
        emit newLog("An unexpected FFmpeg error has occured.", LogUtils::Critical );
        postRenderCleanUp( MediaUtils::Error );
    }
}

void RenderQueue::ffmpegProgress()
{
    //Relay progress information
    _numFrames = _ffmpegRenderer->numFrames();
    _frameRate = _ffmpegRenderer->frameRate();
    _currentFrame = _ffmpegRenderer->currentFrame();
    _startTime = _ffmpegRenderer->startTime();
    _outputSize = _ffmpegRenderer->outputSize();
    _outputBitrate = _ffmpegRenderer->outputBitrate();
    _expectedSize = _ffmpegRenderer->expectedSize();
    _encodingSpeed = _ffmpegRenderer->encodingSpeed();
    _remainingTime = _ffmpegRenderer->timeRemaining();
    _elapsedTime = _ffmpegRenderer->elapsedTime();
    emit progress();
}

void RenderQueue::renderFFmpeg(QueueItem *item)
{
    setStatus( MediaUtils::Launching );
    //generate arguments
    QStringList arguments;
    arguments << "-loglevel" << "error" << "-stats" << "-y";

    //output checks
    MediaInfo *o = item->getOutputMedias()[0];
    FFPixFormat *outputPixFmt = nullptr;
    FFPixFormat::ColorSpace outputColorSpace = FFPixFormat::OTHER;

    if (o->hasVideo())
    {
        outputPixFmt = o->videoStreams()[0]->pixFormat();
        if (outputPixFmt->name() == "") outputPixFmt = o->videoStreams()[0]->defaultPixFormat();
        if (outputPixFmt->name() == "") outputPixFmt = o->defaultPixFormat();
        if (outputPixFmt->name() != "") outputColorSpace = outputPixFmt->colorSpace();
    }

    //input checks
    bool exrInput = false;
    MediaInfo *i = item->getInputMedias()[0];
    if (i->hasVideo()) exrInput = i->videoStreams()[0]->codec()->name() == "exr";

    //some values to be passed from the input to the output
    double inputFramerate = 0.0;

    //add inputs
    foreach(MediaInfo *input, item->getInputMedias())
    {
        QString inputFileName = input->fileName();
        //add custom options
        foreach(QStringList option,input->ffmpegOptions())
        {
            arguments << option[0];
            if (option.count() > 1)
            {
                if (option[1] != "") arguments << option[1];
            }
        }
        if (input->hasVideo())
        {
            VideoInfo *stream = input->videoStreams()[0];

            if (stream->framerate() != 0.0) inputFramerate = stream->framerate();

            //add sequence options
            if (input->isSequence())
            {
                arguments << "-start_number" << QString::number(input->startNumber());
                if (stream->framerate() != 0.0) arguments << "-framerate" << QString::number(stream->framerate());
                else arguments << "-framerate" << "24";
                inputFileName = input->ffmpegSequenceName();
            }

            //add color management
            FFPixFormat::ColorSpace inputColorSpace = FFPixFormat::OTHER;
            FFPixFormat *inputPixFmt = stream->pixFormat();
            if (inputPixFmt->name() == "") inputPixFmt = stream->defaultPixFormat();
            if (inputPixFmt->name() == "") inputPixFmt = input->defaultPixFormat();
            inputColorSpace = inputPixFmt->colorSpace();

            bool convertToYUV = outputColorSpace == FFPixFormat::YUV && inputColorSpace != FFPixFormat::YUV  && input->hasVideo() ;

            if (stream->colorTRC()->name() != "" ) arguments << "-color_trc" << stream->colorTRC()->name();
            else if ( convertToYUV ) arguments << "-color_trc" << "bt709";
            if (stream->colorRange()->name() != "") arguments << "-color_range" << stream->colorRange()->name();
            else if ( convertToYUV ) arguments << "-color_range" << "tv";
            if (stream->colorPrimaries()->name() != "") arguments << "-color_primaries" << stream->colorPrimaries()->name();
            else if ( convertToYUV ) arguments << "-color_primaries" << "bt709";
            if (stream->colorSpace()->name() != "") arguments << "-colorspace" << stream->colorSpace()->name();
            else if ( convertToYUV ) arguments << "-colorspace" << "bt709";
        }

        //time range
        if (input->inPoint() != 0.0) arguments << "-ss" << QString::number(input->inPoint());
        if (input->outPoint() != 0.0) arguments << "-to" << QString::number(input->outPoint());

        //add input file
        arguments << "-i" << QDir::toNativeSeparators(inputFileName);
    }
    //add outputs
    foreach(MediaInfo *output, item->getOutputMedias())
    {
        //maps
        foreach (StreamReference map, output->maps())
        {
            int mediaId = map.mediaId();
            int streamId = map.streamId();
            if (mediaId >= 0 && streamId >= 0) arguments << "-map" << QString::number( mediaId ) + ":" + QString::number( streamId );
        }

        //muxer
        QString muxer = output->muxer()->name();
        if (output->muxer()->isSequence()) muxer = "image2";

        if (muxer != "") arguments << "-f" << muxer;

        //add custom options
        foreach(QStringList option, output->ffmpegOptions())
        {
            QString opt = option[0];
            //Ignore video filters
            if (opt == "-filter:v" || opt == "-vf") continue;
            arguments << opt;
            if (option.count() > 1)
            {
                if (option[1] != "") arguments << option[1];
            }
        }

        //video
        if (output->hasVideo())
        {
            VideoInfo *stream = output->videoStreams()[0];

            FFCodec *vc = stream->codec();
            if (vc->name() != "") arguments << "-c:v" << vc->name();
            else vc = output->defaultVideoCodec();

            if (vc->name() != "copy")
            {
                //bitrate
                int bitrate = int( stream->bitrate() );
                if (bitrate != 0) arguments << "-b:v" << QString::number(bitrate);

                //bitrate type
                if (vc->useBitrateType() && stream->bitrateType() == MediaUtils::BitrateType::CBR)
                {
                    arguments << "-minrate" << QString::number(bitrate);
                    arguments << "-maxrate" << QString::number(bitrate);
                    arguments << "-bufsize" << QString::number(bitrate*2);
                }

                //framerate
                if (stream->framerate() != 0.0) arguments << "-r" << QString::number(stream->framerate());

                //loop (gif)
                if (vc->name() == "gif")
                {
                    int loop = output->loop();
                    arguments << "-loop" << QString::number(loop);
                }

                //level
                if (stream->level() != "") arguments << "-level" << stream->level();

                //quality
                int quality = stream->quality();
                if ( quality >= 0 && vc->name() != "" && vc->qualityParam() != "")
                {
                    arguments << vc->qualityParam() << vc->qualityValue(quality);
                }

                //encoding speed
                int speed = stream->encodingSpeed();
                if (vc->useSpeed() && speed >= 0) arguments << vc->speedParam() << vc->speedValue(speed);

                //fine tuning
                if (vc->useTuning() && stream->tuning()->name() != "")
                {
                    arguments << "-tune" << stream->tuning()->name();
                    if (stream->tuning()->name() == "zerolatency")
                    {
                        arguments << "-movflags" << "+faststart";
                    }
                }

                //start number (sequences)
                if (muxer == "image2")
                {
                    int startNumber = output->startNumber();
                    arguments << "-start_number" << QString::number(startNumber);
                }

                //pixel format
                FFPixFormat *pixFormat = stream->pixFormat();
                QString pixFmt = pixFormat->name();
                //set default for h264 to yuv420 (ffmpeg generates 444 by default which is not standard)
                if (pixFmt == "" && (vc->name() == "h264" || vc->name() == "libx264") ) pixFmt = "yuv420p";
                if (pixFmt == "" && muxer == "mp4") pixFmt = "yuv420p";
                if (pixFmt != "") arguments << "-pix_fmt" << pixFmt;
                // video codecs with alpha need to set -auto-alt-ref to 0
                if (stream->pixFormat()->hasAlpha() && muxer != "image2") arguments << "-auto-alt-ref" << "0";

                //profile
                if (stream->profile()->name() != "" && vc->useProfile())
                {
                    QString profile = stream->profile()->name();
                    // Adjust h264 main profile
                    if ( (vc->name() == "h264" || vc->name() == "libx264") && profile == "high")
                    {
                        if (stream->pixFormat()->name() != "")
                        {
                            if (pixFormat->yuvComponentsDistribution() == "444") profile = "high444";
                            else if (pixFormat->yuvComponentsDistribution() == "422") profile = "high422";
                            else if (pixFormat->bitsPerPixel() > 12) profile = "high10"; // 12 bpp at 420 is 8bpc
                        }
                    }
                    arguments << "-profile:v" << profile;
                }

                // h265 options
                if (vc->name() == "hevc" || vc->name() == "h265" || vc->name() == "libx265")
                {
                    QStringList x265options;
                    if (stream->intra())
                    {
                        float ffmepgVersion = FFmpeg::instance()->version().left(3).toFloat() ;
                        if (ffmepgVersion >= 4.3)
                        {
                            arguments << "-g" << "1";
                        }
                        else
                        {
                            x265options << "keyint=1";
                        }
                    }
                    if (stream->lossless() && stream->quality() >= 0)
                    {
                        x265options << "lossless=1";
                    }
                    if (x265options.count() > 0)
                    {
                        arguments << "-x265-params" << "\"" + x265options.join(":") + "\"";
                    }
                }

                // h264 intra
                if (vc->name() == "h264" || vc->name() == "libx264")
                {
                    if (stream->intra())
                    {
                        float ffmepgVersion = FFmpeg::instance()->version().left(3).toFloat() ;
                        if (ffmepgVersion >= 4.3)
                        {
                            arguments << "-g" << "1";
                        }
                        else
                        {
                            arguments << "-intra";
                        }
                    }

                }

                //color
                //add color management
                if (stream->colorTRC()->name() != "") arguments << "-color_trc" << stream->colorTRC()->name();
                if (stream->colorRange()->name() != "") arguments << "-color_range" << stream->colorRange()->name();
                if (stream->colorTRC()->name() != "") arguments << "-color_primaries" << stream->colorPrimaries()->name();
                if (stream->colorSpace()->name() != "") arguments << "-colorspace" << stream->colorSpace()->name();

                //b-pyramids
                //set as none to h264: not really useful (only on very static footage), but has compatibility issues
                if (vc->name() == "h264") arguments << "-x264opts" << "b_pyramid=0";

                //Vendor
                //set to ap10 with prores for improved compatibility
                if (vc->name().indexOf("prores") >= 0) arguments << "-vendor" << "ap10";

                // ============ Video filters
                QStringList filterChain;

                //unpremultiply
                bool unpremultiply = !stream->premultipliedAlpha();
                if (unpremultiply) filterChain << "unpremultiply=inplace=1";

                //deinterlace
                if (stream->deinterlace())
                {
                    QString deinterlaceOption = "yadif=parity=";
                    if (stream->deinterlaceParity() == MediaUtils::TopFieldFirst) deinterlaceOption += "0";
                    else if (stream->deinterlaceParity() == MediaUtils::BottomFieldFirst) deinterlaceOption += "1";
                    else deinterlaceOption += "-1";
                    filterChain << deinterlaceOption;
                }

                //speed
                if (stream->speed() != 1.0) filterChain << "setpts=" + QString::number(1/stream->speed()) + "*PTS";

                //motion interpolation
                if (stream->speedInterpolationMode() != MediaUtils::NoMotionInterpolation)
                {
                    QString speedFilter = "minterpolate='";
                    if (stream->speedInterpolationMode() == MediaUtils::DuplicateFrames) speedFilter += "mi_mode=dup";
                    else if (stream->speedInterpolationMode() == MediaUtils::BlendFrames) speedFilter += "mi_mode=blend";
                    else
                    {
                        speedFilter += "mi_mode=mci";
                        if (stream->speedInterpolationMode() == MediaUtils::MCIO) speedFilter += ":mc_mode=obmc";
                        else if (stream->speedInterpolationMode() == MediaUtils::MCIAO) speedFilter += ":mc_mode=aobmc";
                        if (stream->speedEstimationMode()->name() != "") speedFilter += ":me_mode=" + stream->speedEstimationMode()->name();
                        if (stream->speedAlgorithm()->name() != "") speedFilter += ":me=" + stream->speedAlgorithm()->name();
                    }
                    if (!stream->sceneDetection()) speedFilter += ":scd=none";
                    else speedFilter += ":scd=fdiff";
                    double framerate = stream->framerate();
                    //get framerate from input
                    if (framerate == 0.0) framerate = inputFramerate;
                    if (framerate > 0.0) speedFilter += ":fps=" + QString::number(framerate);
                    speedFilter += "'";
                    filterChain << speedFilter;
                }

                //crop
                if (!stream->cropUseSize() && ( stream->topCrop() != 0 || stream->bottomCrop() != 0 || stream->leftCrop() != 0 || stream->rightCrop() != 0))
                {
                    QString left = QString::number(stream->leftCrop());
                    QString right = QString::number(stream->rightCrop());
                    QString top = QString::number(stream->topCrop());
                    QString bottom = QString::number(stream->bottomCrop());
                    filterChain << "crop=in_w-" + left + "-" + right + ":in_h-" + top + "-" + bottom + ":" + left + ":" + top;
                }
                else if (stream->cropUseSize() && ( stream->cropHeight() != 0 || stream->cropWidth() != 0) )
                {
                    int wi = stream->cropWidth();
                    int hi = stream->cropHeight();
                    QString w = QString::number( wi );
                    if (wi == 0) w = "in_w";
                    QString h = QString::number( hi );
                    if (hi == 0) h = "in_h";
                    filterChain << "crop=" + w + ":" + h;
                }

                //Collect custom
                foreach(QStringList option, output->ffmpegOptions())
                {
                    QString opt = option[0];
                    //Get video filters
                    if (opt != "-filter:v" && opt != "-vf") continue;
                    if (option.count() > 1)
                    {
                        if (option[1] != "") filterChain << option[1];
                    }
                }

                //LUT for input EXR
                if (exrInput) filterChain << "lutrgb=r=gammaval(0.416666667):g=gammaval(0.416666667):b=gammaval(0.416666667)";

                //1D / 3D LUT
                if (stream->lut() != "")
                {
                    bool filterName = "lut3d";
                    QString lutName = stream->lut();
                    //check if it's 3D or 1D
                    if (lutName.endsWith(".cube"))
                    {
                        QFile l(lutName);
                        if (l.open(QIODevice::ReadOnly | QIODevice::Text))
                        {
                             QTextStream in(&l);
                             QString line = in.readLine();
                             while (!line.isNull())
                             {
                                 if (line.trimmed().toUpper().startsWith("LUT_1D"))
                                 {
                                     filterName = "lut1d";
                                     break;
                                 }
                                 line = in.readLine();
                             }
                             l.close();
                        }
                    }
                    filterChain << "lut1d='" + FFmpeg::escapeFilterOption( stream->lut().replace("\\","/") ) + "'";
                }

                //size
                int width = stream->width();
                int height = stream->height();
                //fix odd sizes
                if (width % 2 != 0)
                {
                    width--;
                    emit newLog("Adjusting width for better compatibility. New width: " + QString::number(width));
                }
                if (height % 2 != 0)
                {
                    height--;
                    emit newLog("Adjusting height for better compatibility. New height: " + QString::number(height));
                }
                if (width != 0 || height != 0)
                {
                    QString w = QString::number(width);
                    QString h = QString::number(height);
                    if (width == 0) w = "in_w";
                    if (height == 0) h = "in_h";
                    QString resizeAlgo = "";
                    if (stream->resizeAlgorithm()->name() != "") resizeAlgo = ":flags=" + stream->resizeAlgorithm()->name();

                    if (stream->resizeMode() == MediaUtils::Stretch)
                    {
                        filterChain << "scale=" + w + ":" + h + resizeAlgo;
                        //we need to set the pixel aspect ratio back to 1:1 to force ffmpeg to stretch
                        filterChain << "setsar=1:1";
                    }
                    else if (stream->resizeMode() == MediaUtils::Crop)
                    {
                        //first resize but keeping ratio (increase)
                        filterChain << "scale=w=" + w + ":h=" + h + ":force_original_aspect_ratio=increase" + resizeAlgo;
                        //then crop what's to large
                        filterChain << "crop=" + w + ":" + h;
                    }
                    else if (stream->resizeMode() == MediaUtils::Letterbox)
                    {
                        //first resize but keeping ratio (decrease)
                        filterChain << "scale=w=" + w + ":h=" + h + ":force_original_aspect_ratio=decrease" + resizeAlgo;
                        //then pad with black bars
                        filterChain << "pad=" + w + ":" + h + ":(ow-iw)/2:(oh-ih)/2";
                    }
                }


                //compile filters
                if (filterChain.count() > 0) arguments << "-vf" << filterChain.join(",");
            }
        }
        else
        {
             arguments << "-vn";
        }

        //audio
        if (output->hasAudio())
        {
            AudioInfo *stream = output->audioStreams()[0];

            QString acodec = stream->codec()->name();
            if (acodec == "")

            //codec
            if (acodec != "") arguments << "-c:a" << acodec;

            if (acodec != "copy")
            {
                //bitrate
                int bitrate = int( stream->bitrate() );
                if (bitrate != 0)
                {
                    arguments << "-b:a" << QString::number(stream->bitrate());
                }

                //sampling
                int sampling = stream->samplingRate();
                if (sampling != 0)
                {
                    arguments << "-ar" << QString::number(sampling);
                }

                //sample format
                QString sampleFormat = stream->sampleFormat()->name();
                if (sampleFormat != "")
                {
                    arguments << "-sample_fmt" << sampleFormat;
                }
            }
        }
        else
        {
            //no audio
            arguments << "-an";
        }

        //file
        QString outputPath = QDir::toNativeSeparators( output->fileName() );

        //if sequence, digits
        if (output->isSequence())
        {
            outputPath = QDir::toNativeSeparators( output->ffmpegSequenceName() );
        }

        arguments << outputPath;
    }

    emit newLog("Beginning new encoding\nUsing FFmpeg commands:\n" + arguments.join(" | "));

    //launch
    _ffmpegRenderer->setOutputFileName( item->getOutputMedias()[0]->fileName() );

    foreach (MediaInfo *m, item->getInputMedias())
    {
        if (m->hasVideo())
        {
            VideoInfo *stream = m->videoStreams()[0];
            _ffmpegRenderer->setNumFrames( int( m->duration() * stream->framerate() ) );
            _ffmpegRenderer->setFrameRate( stream->framerate() );
            break;
        }
    }

    _ffmpegRenderer->start( arguments );
}

void RenderQueue::aeLog(QString message, LogUtils::LogType lt)
{
    message = "After Effects | " + message;
    emit newLog( message, lt );
}

void RenderQueue::aeProgress()
{
    //Relay progress information
    _numFrames = _aeRenderer->numFrames();
    _frameRate = _aeRenderer->frameRate();
    _currentFrame = _aeRenderer->currentFrame();
    _startTime = _aeRenderer->startTime();
    _outputSize = _aeRenderer->outputSize();
    _outputBitrate = _aeRenderer->outputBitrate();
    _expectedSize = _aeRenderer->expectedSize();
    _encodingSpeed = _aeRenderer->encodingSpeed();
    _remainingTime = _aeRenderer->timeRemaining();
    _elapsedTime = _aeRenderer->elapsedTime();
    emit progress();
}

QTime RenderQueue::elapsedTime() const
{
    return _elapsedTime;
}

QTime RenderQueue::remainingTime() const
{
    return _remainingTime;
}

double RenderQueue::encodingSpeed() const
{
    return _encodingSpeed;
}

double RenderQueue::expectedSize() const
{
    return _expectedSize;
}

double RenderQueue::outputBitrate() const
{
    return _outputBitrate;
}

double RenderQueue::outputSize( ) const
{
    return _outputSize;
}

int RenderQueue::currentFrame() const
{
    return _currentFrame;
}

int RenderQueue::numFrames() const
{
    return _numFrames;
}

MediaUtils::RenderStatus RenderQueue::status() const
{
    return _status;
}

QueueItem *RenderQueue::currentItem()
{
    return _currentItem;
}

void RenderQueue::encode()
{
    if (_status == MediaUtils::FFmpegEncoding || _status == MediaUtils::AERendering  || _status == MediaUtils::BlenderRendering ) return;

    setStatus( MediaUtils::Launching );
    //launch first item
    encodeNextItem();
}

void RenderQueue::encode(QueueItem *item)
{
    _encodingQueue.clear();
    _encodingQueue << item;
    encode();
}

void RenderQueue::encode(QList<QueueItem*> list)
{
    _encodingQueue = list;
    encode();
}

void RenderQueue::encode(QList<MediaInfo *> inputs, QList<MediaInfo *> outputs)
{
    QueueItem *item = new QueueItem(inputs,outputs,this);
    _encodingQueue.clear();
    _encodingQueue << item;
    encode();
}

void RenderQueue::encode(MediaInfo *input, MediaInfo *output)
{
    QueueItem *item = new QueueItem(input,output,this);
    _encodingQueue.clear();
    _encodingQueue << item;
    encode();
}

int RenderQueue::addQueueItem(QueueItem *item)
{
    _encodingQueue << item;
    return _encodingQueue.count()-1;
}

void RenderQueue::removeQueueItem(int id)
{
    QueueItem *i = _encodingQueue.takeAt(id);
    i->deleteLater();
}

QueueItem *RenderQueue::takeQueueItem(int id)
{
    return _encodingQueue.takeAt(id);
}

void RenderQueue::clearQueue()
{
    while(_encodingQueue.count() > 0)
    {
        removeQueueItem(0);
    }
}

void RenderQueue::stop(int timeout)
{
    emit newLog( "Stopping queue" );

    if ( _status == MediaUtils::FFmpegEncoding )
    {
        _ffmpegRenderer->stop( timeout );
    }
    else if ( _status == MediaUtils::AERendering )
    {
        _aeRenderer->stop( timeout );
    }

    setStatus( MediaUtils::Waiting );

    emit newLog( "Queue stopped" );
}

void RenderQueue::postRenderCleanUp( MediaUtils::RenderStatus lastStatus )
{
    if (_status == MediaUtils::FFmpegEncoding || _status == MediaUtils::AERendering || _status == MediaUtils::BlenderRendering )
    {
        setStatus( MediaUtils::Cleaning );

        //restore ae templates
        if ( _status == MediaUtils::AERendering ) _ae->restoreOriginalTemplates();

        finishCurrentItem( lastStatus );

        encodeNextItem();
    }
    else
    {
        setStatus( lastStatus );
    }
}

void RenderQueue::encodeNextItem()
{
    if (_encodingQueue.count() == 0)
    {
        setStatus( MediaUtils::Waiting );
        return;
    }

    _currentItem = _encodingQueue.takeAt(0);
    //connect item status to queue status
    connect(this, SIGNAL(statusChanged(MediaUtils::RenderStatus)), _currentItem, SLOT(statusChanged(MediaUtils::RenderStatus)) );

    setStatus( MediaUtils::Launching );

    //Check if there are AEP to render
    foreach(MediaInfo *input, _currentItem->getInputMedias())
    {
        if (input->isAep())
        {
            //check if we need audio
            bool needAudio = false;
            foreach(MediaInfo *output, _currentItem->getOutputMedias())
            {
                if (output->hasAudio())
                {
                    needAudio = true;
                    break;
                }
            }

            renderAep(input, needAudio);
            return;
        }
    }

    //Now all aep are rendered, transcode with ffmpeg
    renderFFmpeg( _currentItem );
}

void RenderQueue::finishCurrentItem( MediaUtils::RenderStatus lastStatus )
{
    if (_currentItem == nullptr) return;
    //disconnect item status from queue status
    disconnect(this, nullptr, _currentItem, nullptr);
    _currentItem->setStatus( lastStatus );
    _currentItem->postRenderCleanUp();
    //move to history
    _encodingHistory << _currentItem;
    _currentItem = nullptr;
}

void RenderQueue::aeStatusChanged( MediaUtils::RenderStatus status )
{
    if ( MediaUtils::isBusy( status ) )
    {
        setStatus( MediaUtils::AERendering );
        emit newLog("After Effects is running.");
    }
    else if ( status == MediaUtils::Finished )
    {
        MediaInfo *input = _currentItem->getInputMedias()[0];

        emit newLog("After Effects Render process successfully finished");

        //encode rendered EXR
        if (!input->aeUseRQueue())
        {
            //set exr
            //get one file
            QString aeTempPath = input->cacheDir()->path();
            QDir aeTempDir(aeTempPath);
            QStringList filters("DuME_*.exr");
            QStringList files = aeTempDir.entryList(filters,QDir::Files | QDir::NoDotAndDotDot);

           //if nothing has been rendered, set to error and go on with next queue item
            if (files.count() == 0)
            {
                postRenderCleanUp( MediaUtils::Error );
                return;
            }

            //set file and launch
            //frames
            double frameRate = input->videoStreams()[0]->framerate();
            input->update( QFileInfo(aeTempPath + "/" + files[0]));
            if (int( frameRate ) != 0) input->videoStreams()[0]->setFramerate(frameRate);
            //add audio
            QFileInfo audioFile(aeTempPath + "/DuME.wav");
            if (audioFile.exists())
            {
                MediaInfo *audio = new MediaInfo(audioFile, this);
                _currentItem->addInputMedia(audio);
            }

            //reInsert at first place in renderqueue
            _encodingQueue.insert(0,_currentItem);
            //and go
            encodeNextItem();
        }
        else
        {
            emit newLog("After Effects Rendering process successfully finished.");
            postRenderCleanUp( MediaUtils::Finished );
        }
    }
    else if ( status == MediaUtils::Stopped )
    {
        emit newLog("After Effects rendering has been stopped.");
        postRenderCleanUp( MediaUtils::Stopped );
    }
    else if ( status == MediaUtils::Error )
    {
        emit newLog("An unexpected After Effects error has occured.", LogUtils::Critical);
        postRenderCleanUp( MediaUtils::Error );
    }
}

void RenderQueue::renderAep(MediaInfo *input, bool audio)
{
    QStringList arguments("-project");
    QStringList audioArguments;
    arguments <<  QDir::toNativeSeparators(input->fileName());

    QString tempPath = "";

    //if not using the existing render queue
    if (!input->aeUseRQueue())
    {
        //set the cache dir
        QTemporaryDir *aeTempDir = CacheManager::instance()->getAeTempDir();
        emit newLog( "After Effects temporary dir set to: " + QDir::toNativeSeparators(aeTempDir->path()) );
        input->setCacheDir(aeTempDir);

        //if a comp name is specified, render this comp
        if (input->aepCompName() != "") arguments << "-comp" << input->aepCompName();
        //else use the sepecified renderqueue item
        else if (input->aepRqindex() > 0) arguments << "-rqindex" << QString::number(input->aepRqindex());
        //or the first one if not specified
        else arguments << "-rqindex" << "1";

        //and finally, append arguments
        audioArguments = arguments;

        arguments << "-RStemplate" << "DuMultiMachine";
        arguments << "-OMtemplate" << "DuEXR";
        tempPath = QDir::toNativeSeparators(aeTempDir->path() + "/" + "DuME_[#####]");
        arguments << "-output" << tempPath;

        if (audio)
        {
            audioArguments << "-RStemplate" << "DuBest";
            audioArguments << "-OMtemplate" << "DuWAV";
            QString audioTempPath = QDir::toNativeSeparators(aeTempDir->path() + "/" + "DuME");
            audioArguments << "-output" << audioTempPath;
        }
        else
        {
            audioArguments.clear();
        }

        // Add our templates for rendering
        _ae->setDuMETemplates();
    }

    emit newLog("Beginning After Effects rendering\nUsing aerender commands:\n" + arguments.join(" | "));

    //adjust the number of threads
    //keep one available for exporting the audio
    int numThreads = input->aepNumThreads();
    if (audio && numThreads > 1) numThreads--;

    // video
    _aeRenderer->setNumFrames( int( input->duration() * input->videoStreams()[0]->framerate() ) );
    _aeRenderer->setFrameRate( input->videoStreams()[0]->framerate() );
    _aeRenderer->setOutputFileName( tempPath );
    _aeRenderer->start( arguments, numThreads );
    // audio
    if (audio) _aeRenderer->start( audioArguments );

    setStatus( MediaUtils::AERendering );
}
