#include "queuewidget.h"

#ifdef QT_DEBUG
#include <QtDebug>
#endif

QueueWidget::QueueWidget(FFmpeg *ff,QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    ffmpeg = ff;

    inputWidget = new InputWidget(ffmpeg,this);
    outputWidget = new OutputWidget(ffmpeg,this);

    inputTab1->layout()->addWidget(inputWidget);
    outputTab1->layout()->addWidget(outputWidget);

    ffmpeg_init();

    connect(ffmpeg,SIGNAL(binaryChanged()),this,SLOT(ffmpeg_init()));

    connect(inputWidget,SIGNAL(newMediaLoaded(FFMediaInfo*)),outputWidget,SLOT(newInputMedia(FFMediaInfo*)));
}

FFMediaInfo *QueueWidget::getInputMedia()
{
    return inputWidget->getMediaInfo();
}

FFMediaInfo *QueueWidget::getOutputMedia()
{
    return outputWidget->getMediaInfo();
}

void QueueWidget::ffmpeg_init()
{

}

void QueueWidget::presetsPathChanged(QString path)
{
    outputWidget->loadPresets(path);
}

void QueueWidget::saveSettings()
{

}
