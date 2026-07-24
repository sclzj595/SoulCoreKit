#ifndef SOUL_UI_ICON_MANAGER_H
#define SOUL_UI_ICON_MANAGER_H

#include <QObject>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QMap>
#include <QColor>
#include "soul/ui/design_constants.h"
#include "soul/base/base_manager.h"

namespace sc {

class IconManager : public BaseManager {
    Q_OBJECT
public:
    static IconManager& instance();

    QIcon icon(const QString& name, const QColor& color = QColor(), int size = design::DEFAULT_ICON_SIZE);
    QPixmap pixmap(const QString& name, const QColor& color = QColor(), int size = design::DEFAULT_ICON_SIZE);

    void registerIcon(const QString& name, const QString& path);
    void registerIcon(const QString& name, const QIcon& icon);

    void setDefaultColor(const QColor& color);
    QColor defaultColor() const;

    void setDefaultSize(int size);
    int defaultSize() const;

    bool init() override;
    void shutdown() override;

private:
    IconManager();
    ~IconManager() override = default;

    QPixmap applyColorToPixmap(const QPixmap& source, const QColor& color);

    QMap<QString, QString> m_iconPaths;
    QMap<QString, QIcon> m_icons;
    QColor m_defaultColor;
    int m_defaultSize;
};

}

#endif