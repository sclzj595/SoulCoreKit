#ifndef SOUL_UTILS_CLIPBOARD_UTILS_H
#define SOUL_UTILS_CLIPBOARD_UTILS_H

#include <QString>
#include <QImage>
#include <QUrl>

namespace sc {

class ClipboardUtils {
public:
    static bool setText(const QString& text);
    static QString getText();

    static bool setHtml(const QString& html);
    static QString getHtml();

    static bool setImage(const QImage& image);
    static QImage getImage();

    static bool setUrls(const QList<QUrl>& urls);
    static QList<QUrl> getUrls();

    static bool hasText();
    static bool hasImage();
    static bool hasUrls();

    static void clear();
};

}

#endif