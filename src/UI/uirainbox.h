#ifndef RAINBOXUI_H
#define RAINBOXUI_H

#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QApplication>

class UIRainbox
{
public:
    UIRainbox();
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
};

#endif // RAINBOXUI_H
