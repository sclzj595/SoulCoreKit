#ifndef SOUL_UTILS_IMAGE_UTILS_H
#define SOUL_UTILS_IMAGE_UTILS_H

#include <QPixmap>
#include <QImage>
#include <QString>

namespace sc {

class ImageUtils {
public:
    static QPixmap resize(const QPixmap& pixmap, int width, int height, bool keepAspectRatio = true);
    static QImage resize(const QImage& image, int width, int height, bool keepAspectRatio = true);

    static QPixmap scale(const QPixmap& pixmap, qreal factor);
    static QImage scale(const QImage& image, qreal factor);

    static QImage toGrayscale(const QImage& image);
    static QImage toSepia(const QImage& image);

    static bool save(const QImage& image, const QString& path, const QString& format = "PNG");
    static QImage load(const QString& path);

    static QPixmap blur(const QPixmap& pixmap, int radius = 10);
    static QImage blur(const QImage& image, int radius = 10);

    static QImage crop(const QImage& image, const QRect& rect);
    static QPixmap crop(const QPixmap& pixmap, const QRect& rect);

    static QImage rotate(const QImage& image, int degrees);
    static QPixmap rotate(const QPixmap& pixmap, int degrees);

    static QColor averageColor(const QImage& image);
};

}

#endif