﻿#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include <QObject>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryDir>
#include <QtDebug>

#include "FFmpeg/ffmpeg.h"
#include "utils.cpp"

class MediaInfo : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief DuMediaInfo Constructs an empty MediaInfo
     * @param parent The QObject parent
     */
    explicit MediaInfo( FFmpeg *ffmpeg, QObject *parent = nullptr );

    /**
     * @brief DuMediaInfo Constructs a new MediaInfo for the given file.
     * @param mediaFile The media file or a JSON preset. For a frame sequence, it can be any frame from the sequence
     * @param parent The QObject parent
     */
    explicit MediaInfo( FFmpeg *ffmpeg, QFileInfo mediaFile, QObject *parent = nullptr );


    //setters

    /**
     * @brief updateInfo Updates all the information for this media file
     * @param mediaFilePath The media file. For a frame sequence, it can be any frame from the sequence
     */
    void updateInfo(QFileInfo mediaFile, bool silent = false);
    void updateInfo(MediaInfo *other, bool updateFilename = false, bool silent = false);
    void loadPreset(QFileInfo presetFile, bool silent = false);
    /**
     * @brief reInit Reinit all to default values
     */
    void reInit(bool removeFileName = true, bool silent = false);

    void setMuxer(FFMuxer *muxer, bool silent = false);
    void setMuxer(QString name, bool silent = false);
    void setContainer(QStringList container, bool silent = false);
    void setVideoWidth(int width, bool silent = false);
    void setVideoHeight(int height, bool silent = false);
    void setVideoFramerate(double fps, bool silent = false);
    void setAudioSamplingRate(int sampling, bool silent = false);
    void setDuration(double duration, bool silent = false);
    void setFileName(QString fileName, bool silent = false);
    void setVideoCodec(FFCodec *codec, bool silent = false);
    void setVideoCodec(QString codecName, bool silent = false);
    void setAudioCodec(FFCodec *codec, bool silent = false);
    void setAudioCodec(QString codecName, bool silent = false);
    void setVideoBitrate(qint64 bitrate, bool silent = false);
    void setAudioBitrate(qint64 bitrate, bool silent = false);
    void setSize(double size, MediaUtils::SizeUnit unit = MediaUtils::Bytes, bool silent = false);
    void setFFmpegOptions(QList<QStringList> options, bool silent = false);
    void setVideo(bool video = true, bool silent = false);
    void setAudio(bool audio = true, bool silent = false);
    void setVideoProfile(int profile, bool silent = false);
    void setVideoQuality(int quality, bool silent = false);
    void setLoop(int loop, bool silent = false);
    void addFFmpegOption(QStringList option, bool silent = false);
    void removeFFmpegOptions(QString optionName, bool silent = false);
    void clearFFmpegOptions(bool silent = false);
    void setStartNumber(int startNumber, bool silent = false);
    void setFrames(const QStringList &frames, bool silent = false);
    void setPixFormat(FFPixFormat *pixFormat, bool silent = false);
    void setPixFormat(QString name, bool silent = false);
    void setPremultipliedAlpha(bool premultipliedAlpha, bool silent = false);
    void setTrc(const QString &trc, bool silent = false);
    void setAep(bool isAep, bool silent = false);
    void setAepNumThreads(int aepNumThreads, bool silent = false);
    void setAepCompName(const QString &aepCompName, bool silent = false);
    void setAepRqindex(int aepRqindex, bool silent = false);
    void setCacheDir(QTemporaryDir *cacheDir, bool silent = false);
    void setAeUseRQueue(bool aeUseRQueue, bool silent = false);
    void setAlpha(bool alpha, bool silent = false);
    //getters
    FFMuxer *muxer() const;
    int videoWidth();
    int videoHeight();
    double videoFramerate();
    int audioSamplingRate();
    QString audioChannels() const;
    double duration();
    QString fileName();
    FFCodec *videoCodec();
    FFCodec *defaultVideoCodec() const;
    FFCodec *audioCodec();
    FFCodec *defaultAudioCodec() const;
    FFPixFormat *pixFormat();
    FFPixFormat *defaultPixFormat() const;
    qint64 audioBitrate();
    qint64 videoBitrate();
    qint64 size();
    QList<QStringList> ffmpegOptions();
    bool hasVideo();
    bool hasAudio();
    bool isImageSequence();
    QStringList extensions() const;
    int videoQuality() const;
    int videoProfile() const;
    int loop() const;
    int startNumber() const;
    QStringList frames() const;
    bool premultipliedAlpha() const;
    QString trc() const;
    bool isAep() const;
    QString aepCompName() const;
    int aepNumThreads() const;
    int aepRqindex() const;
    QTemporaryDir *cacheDir() const;
    bool aeUseRQueue() const;
    QString ffmpegSequenceName() const;
    qint64 bitrate() const;
    float pixAspect() const;
    float videoAspect() const;
    bool hasAlpha() const;
    bool canHaveAlpha() const;
    bool copyVideo() const;
    bool copyAudio() const;

    //utils
    QString exportPreset();
    void exportPreset(QString jsonPath);  

signals:
    /**
     * @brief changed Emitted when some parameters have been changed.
     */
    void changed();

public slots:

private:

    // === FOR INTERNAL USE ===

    // The ffmpeg pointer to get all ffmpeg information and analyse medias
    FFmpeg *_ffmpeg;

    // ========== ATTRIBUTES ==============

    // GENERAL

    /**
     * @brief _extensions The list of file extensions this media can use
     */
    QStringList _extensions;
    /**
     * @brief _muxer The muxer of this media file
     */
    FFMuxer *_muxer;
    /**
     * @brief _duration The duration in seconds
     */
    double _duration;
    /**
     * @brief _size The file size
     */
    qint64 _size;
    /**
     * @brief _bitrate The global bitrate
     */
    qint64 _bitrate;
    /**
     * @brief _video Wether the file contains video
     */
    bool _video;
    /**
     * @brief _audio Wether the file contains audio
     */
    bool _audio;
    /**
     * @brief _imageSequence Wether this is a frame sequence instead of a single media file
     */
    bool _imageSequence;
    /**
     * @brief _videoCodec The norm or codec used for decoding or encoding the video
     */
    FFCodec *_videoCodec;
    /**
     * @brief _videoWidth The width in pixels
     */
    int _videoWidth;
    /**
     * @brief _videoHeight The height in pixels
     */
    int _videoHeight;
    /**
     * @brief _videoFramerate The framerate in frames per second
     */
    double _videoFramerate;
    /**
     * @brief _videoBitrate The video bitrate
     */
    qint64 _videoBitrate;
    /**
     * @brief _pixFormat The pixel format
     */
    FFPixFormat *_pixFormat;
    /**
     * @brief _pixRatio Pixel aspect ratio
     */
    float _pixAspect;
    /**
     * @brief _videoAspect Video aspect ratio
     */
    float _videoAspect;
    /**
     * @brief _audioCodec The norm or codec used for decoding or encoding audio
     */
    FFCodec *_audioCodec;
    /**
     * @brief _audioChannels The type of channels: mono/stereo/5.1
     */
    QString _audioChannels;
    /**
     * @brief _audioSamplingRate The audio sampling in kHz
     */
    int _audioSamplingRate;
    /**
     * @brief _audioBitrate The audio bitrate
     */
    qint64 _audioBitrate;
    /**
     * @brief _fileName The filename of the media
     */
    QString _fileName;
    /**
     * @brief _frames For a frame sequence, the list of all the frames filenames
     */
    QStringList _frames;
    /**
     * @brief _videoQuality The video quality, as used by some codecs like h264
     */
    int _videoQuality;
    /**
     * @brief _videoProfile The video profile, as used by some codecs like prores
     */
    int _videoProfile;
    /**
     * @brief _loop The number of loops to be encoded (-1 for inifite, available only with some specific formats like GIF)
     */
    int _loop;
    /**
     * @brief _startNumber The number of the first frame
     */
    int _startNumber;
    /**
     * @brief _premultipliedAlpha Wether the Alpha is premultiplied
     */
    bool _premultipliedAlpha;
    /**
     * @brief _trc The color transfer profile (gamma)
     */
    QString _trc;

    // GENERAL Encoding/decoding parameters

    /**
     * @brief _cacheDir A pointer to the cache for temporary transcoding and other files for this media
     */
    QTemporaryDir *_cacheDir;

    // FFMPEG Encoding/decoding

    /**
     * @brief _ffmpegOptions The custom options to be used by ffmpeg when decoding or encoding this media
     */
    QList<QStringList> _ffmpegOptions;
    /**
     * @brief _sequenceName The name of the file sequence as used by ffmpeg
     */
    QString _ffmpegSequenceName;

    // AFTER EFFECTS Encoding/decoding

    /**
     * @brief _isAep Wether this is an After Effects project (aep, aet or aepx)
     */
    bool _isAep;
    /**
     * @brief _aepNumThreads The number of threads to use when rendering this mediq with After Effects
     */
    int _aepNumThreads;
    /**
     * @brief _aepCompName The name of the composition in this After Effects project to render
     */
    QString _aepCompName;
    /**
     * @brief _aepRqindex The index of the render queue item in this After Effects project to render
     */
    int _aepRqindex;
    /**
     * @brief _aeUseRQueue Wether to launch the render queue when rendering the After Effects project, or render a specific composition or renderqueue item.
     */
    bool _aeUseRQueue;

    // ======== METHODS =============

    /**
     * @brief loadSequence Loads all the frames of the frame sequence
     */
    void loadSequence();

};

#endif // MEDIAINFO_H
