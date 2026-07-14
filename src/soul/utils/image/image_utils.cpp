#include "soul/utils/image/image_utils.h"

namespace sc {

QPixmap ImageUtils::resize(const QPixmap& pixmap, int width, int height, bool keepAspectRatio) {
    if (keepAspectRatio) {
        return pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return pixmap.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QImage ImageUtils::resize(const QImage& image, int width, int height, bool keepAspectRatio) {
    if (keepAspectRatio) {
        return image.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QPixmap ImageUtils::scale(const QPixmap& pixmap, qreal factor) {
    return pixmap.scaled(pixmap.width() * factor, pixmap.height() * factor,
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage ImageUtils::scale(const QImage& image, qreal factor) {
    return image.scaled(image.width() * factor, image.height() * factor,
                        Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage ImageUtils::toGrayscale(const QImage& image) {
    return image.convertToFormat(QImage::Format_Grayscale8);
}

QImage ImageUtils::toSepia(const QImage& image) {
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    for (int y = 0; y < result.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            int r = qRed(line[x]);
            int g = qGreen(line[x]);
            int b = qBlue(line[x]);
            int tr = qMin(255, (int)(0.393 * r + 0.769 * g + 0.189 * b));
            int tg = qMin(255, (int)(0.349 * r + 0.686 * g + 0.168 * b));
            int tb = qMin(255, (int)(0.272 * r + 0.534 * g + 0.131 * b));
            line[x] = qRgb(tr, tg, tb);
        }
    }
    return result;
}

bool ImageUtils::save(const QImage& image, const QString& path, const QString& format) {
    return image.save(path, format.toUtf8().constData());
}

QImage ImageUtils::load(const QString& path) {
    return QImage(path);
}

QPixmap ImageUtils::blur(const QPixmap& pixmap, int radius) {
    Q_UNUSED(radius);
    return pixmap;
}

QImage ImageUtils::blur(const QImage& image, int radius) {
    Q_UNUSED(radius);
    return image;
}

QImage ImageUtils::crop(const QImage& image, const QRect& rect) {
    return image.copy(rect);
}

QPixmap ImageUtils::crop(const QPixmap& pixmap, const QRect& rect) {
    return pixmap.copy(rect);
}

QImage ImageUtils::rotate(const QImage& image, int degrees) {
    QTransform transform;
    transform.rotate(degrees);
    return image.transformed(transform);
}

QPixmap ImageUtils::rotate(const QPixmap& pixmap, int degrees) {
    QTransform transform;
    transform.rotate(degrees);
    return pixmap.transformed(transform);
}

QColor ImageUtils::averageColor(const QImage& image) {
    if (image.isNull()) {
        return QColor();
    }
    int r = 0, g = 0, b = 0;
    int count = 0;
    QImage img = image.convertToFormat(QImage::Format_RGB32);
    for (int y = 0; y < img.height(); ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            r += qRed(line[x]);
            g += qGreen(line[x]);
            b += qBlue(line[x]);
            ++count;
        }
    }
    return QColor(r / count, g / count, b / count);
}

}