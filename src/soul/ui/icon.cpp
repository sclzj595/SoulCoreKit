#include "soul/ui/icon.h"

namespace sc {

QIcon Icon::s_appIcon;

QIcon Icon::fromResource(const QString& path) {
    return QIcon(path);
}

QIcon Icon::fromFont(const QString& fontName, QString character) {
    Q_UNUSED(fontName);
    Q_UNUSED(character);
    return QIcon();
}

QIcon Icon::fromColor(const QColor& color, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(color);
    return QIcon(pixmap);
}

QIcon Icon::appIcon() {
    return s_appIcon;
}

void Icon::setAppIcon(const QIcon& icon) {
    s_appIcon = icon;
}

}