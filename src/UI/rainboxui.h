#ifndef RAINBOXUI_H
#define RAINBOXUI_H

#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QApplication>
#include <QSplashScreen>
#include <QProgressBar>
#include <QLabel>
#include <QFontDatabase>

#include "utils.h"

class RainboxUI
{
public:
    RainboxUI();
    static void updateCSS(QString cssFileName, QString appName = "");
    /**
     * @brief loadCSS Loads a CSS File
     * If any cssFileName-values.val file is found, uses these values in the CSS
     * Note: in the values file name, the .val extension can be omitted, or replaced by .txt
     * @param cssFileName The file name (with complete path) of the CSS
     * @return the CSS string
     */
    static QString loadCSS(QString cssFileName = ":/styles/default");
    /**
     * @brief loadCSS Loads multiple CSS Files
     * If any cssFileName[0]-values.val file is found, uses these values in the CSS
     * Note: in the values file name, the .val extension can be omitted, or replaced by .txt
     * @param cssFileNames The file names (with complete path) of the CSS
     * @return the CSS string
     */
    static QString loadCSS(QStringList cssFileNames);
    static void setFont(QString family = "Calibri");
};

class UISplashScreen : public QSplashScreen
{
public:
    UISplashScreen(const QPixmap &pixmap = QPixmap(), QString version = "", Qt::WindowFlags f = Qt::WindowFlags());

public slots:
    void newMessage(QString message, LogUtils::LogType lt = LogUtils::Information);
    void progressMax(int max);
    void progress(int val);

private:
    QProgressBar *_progressBar;
    QLabel *_versionLabel;
};


#endif // RAINBOXUI_H