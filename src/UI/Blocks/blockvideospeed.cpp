#include "blockvideospeed.h"

BlockVideoSpeed::BlockVideoSpeed(MediaInfo *mediaInfo, QWidget *parent) :
    BlockContentWidget(mediaInfo, parent)
{
#ifdef QT_DEBUG
    qDebug() << "Create video speed block";
#endif
    setType(Type::Video);
    setupUi(this);

    _freezeUI = true;
    interpolationBox->addItem("None");
    interpolationBox->addItem("Duplicate frames");
    interpolationBox->addItem("Blend frames");
    interpolationBox->addItem("Motion compensated - Overlapped");
    interpolationBox->addItem("Motion compensated - Adaptive");
    interpolationBox->setCurrentIndex(0);

    foreach(FFBaseObject *me, FFmpeg::instance()->motionEstimationModes())
    {
        estimationBox->addItem(me->prettyName(), me->name());
    }
    estimationBox->setCurrentIndex(0);

    foreach(FFBaseObject *mia, FFmpeg::instance()->motionInterpolationAlgorithms())
    {
        algorithmBox->addItem(mia->prettyName(), mia->name());
    }
    algorithmBox->setCurrentIndex(0);

    updateInterpolationUI();
    _freezeUI = false;
}

void BlockVideoSpeed::activate(bool activate)
{
    if (!activate)
    {
        _mediaInfo->setVideoSpeed(1.0);

    }
    else
    {
        _mediaInfo->setVideoSpeed(speedBox->value());
    }
}

void BlockVideoSpeed::update()
{
    VideoInfo *stream = _mediaInfo->videoStreams()[0];
    speedBox->setValue(stream->speed());
}

void BlockVideoSpeed::on_speedBox_valueChanged(double arg1)
{
    updateInterpolationUI();
    if (_freezeUI) return;
    _mediaInfo->setVideoSpeed(arg1);
}

void BlockVideoSpeed::updateInterpolationUI()
{
    if (speedBox->value() == 1.0)
    {
        interpolationBox->setEnabled(false);
        estimationBox->setEnabled(false);
        algorithmBox->setEnabled(false);
        return;
    }
    interpolationBox->setEnabled(true);
    int interpoMode = interpolationBox->currentIndex();
    bool interpo = interpoMode == 3 || interpoMode == 4;
    estimationBox->setEnabled(interpo);
    algorithmBox->setEnabled(interpo);
}

void BlockVideoSpeed::on_interpolationBox_currentIndexChanged(int index)
{
    updateInterpolationUI();
    if (_freezeUI) return;
    if (index == 0) _mediaInfo->setVideoSpeedInterpolationMode(MediaUtils::NoMotionInterpolation);
    else if (index == 1) _mediaInfo->setVideoSpeedInterpolationMode(MediaUtils::DuplicateFrames);
    else if (index == 2) _mediaInfo->setVideoSpeedInterpolationMode(MediaUtils::BlendFrames);
    else if (index == 3) _mediaInfo->setVideoSpeedInterpolationMode(MediaUtils::MCIO);
    else if (index == 4) _mediaInfo->setVideoSpeedInterpolationMode(MediaUtils::MCIAO);
}

void BlockVideoSpeed::on_estimationBox_currentIndexChanged(int /*index*/)
{
    if (_freezeUI) return;
    _mediaInfo->setVideoSpeedEstimationMode( estimationBox->currentData().toString() );
}

void BlockVideoSpeed::on_algorithmBox_currentIndexChanged(int /*index*/)
{
    if (_freezeUI) return;
    _mediaInfo->setVideoSpeedAlgorithm( algorithmBox->currentData().toString() );
}