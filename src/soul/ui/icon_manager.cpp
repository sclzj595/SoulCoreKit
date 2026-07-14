#include "soul/ui/icon_manager.h"
#include <QPainter>
#include <QPixmapCache>

namespace sc {

IconManager::IconManager()
    : m_defaultColor(QColor("#1e293b")),
      m_defaultSize(design::DEFAULT_ICON_SIZE) {
}

bool IconManager::init() {
    m_initialized = true;
    return true;
}

void IconManager::shutdown() {
    m_icons.clear();
    m_iconPaths.clear();
    m_initialized = false;
}

IconManager& IconManager::instance() {
    static IconManager instance;
    return instance;
}

QIcon IconManager::icon(const QString& name, const QColor& color, int size) {
    QColor iconColor = color.isValid() ? color : m_defaultColor;
    int iconSize = size > 0 ? size : m_defaultSize;

    QString cacheKey = QString("%1_%2_%3").arg(name).arg(iconColor.name()).arg(iconSize);

    if (QPixmapCache::find(cacheKey, nullptr)) {
        QPixmap pixmap;
        QPixmapCache::find(cacheKey, &pixmap);
        return QIcon(pixmap);
    }

    QPixmap pixmap = this->pixmap(name, iconColor, iconSize);
    QPixmapCache::insert(cacheKey, pixmap);

    return QIcon(pixmap);
}

QPixmap IconManager::pixmap(const QString& name, const QColor& color, int size) {
    QColor iconColor = color.isValid() ? color : m_defaultColor;
    int iconSize = size > 0 ? size : m_defaultSize;

    QPixmap result;

    if (m_icons.contains(name)) {
        result = m_icons[name].pixmap(iconSize, iconSize);
    } else if (m_iconPaths.contains(name)) {
        result.load(m_iconPaths[name]);
        if (!result.isNull()) {
            result = result.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    } else {
        result.load(name);
        if (!result.isNull()) {
            result = result.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }

    if (!result.isNull() && iconColor.isValid()) {
        result = applyColorToPixmap(result, iconColor);
    }

    return result;
}

void IconManager::registerIcon(const QString& name, const QString& path) {
    m_iconPaths[name] = path;
}

void IconManager::registerIcon(const QString& name, const QIcon& icon) {
    m_icons[name] = icon;
}

void IconManager::setDefaultColor(const QColor& color) {
    m_defaultColor = color;
}

QColor IconManager::defaultColor() const {
    return m_defaultColor;
}

void IconManager::setDefaultSize(int size) {
    m_defaultSize = size;
}

int IconManager::defaultSize() const {
    return m_defaultSize;
}

QPixmap IconManager::applyColorToPixmap(const QPixmap& source, const QColor& color) {
    QPixmap result = source.copy();
    QPainter painter(&result);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(result.rect(), color);
    painter.end();
    return result;
}

}