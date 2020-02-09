#ifndef BLOCKCOLOR_H
#define BLOCKCOLOR_H

#include "ui_blockcolor.h"
#include "UI/Blocks/blockcontentwidget.h"

class BlockColor : public BlockContentWidget, private Ui::BlockColor
{
    Q_OBJECT

public:
    explicit BlockColor(MediaInfo *mediaInfo, QWidget *parent = nullptr);
public slots:
    void activate( bool blockEnabled );
    void update();
private slots:
    void on_spaceBox_currentIndexChanged(int index);
    void on_primariesBox_currentIndexChanged(int index);
    void on_trcBox_currentIndexChanged(int index);
    void on_rangeBox_currentIndexChanged(int index);
    void presetTriggered();
    void on_actionsRGB_triggered();
    void on_actionBT709_triggered();
    void on_actionUHD10_triggered();
    void on_actionUHD12_triggered();
};

#endif // BLOCKCOLOR_H
