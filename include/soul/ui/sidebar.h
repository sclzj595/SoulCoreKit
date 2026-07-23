#ifndef SOUL_UI_SIDEBAR_H
#define SOUL_UI_SIDEBAR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QList>
#include <QString>
#include <QIcon>

namespace sc {

struct SideBarItem {
    QString id;
    QString text;
    QIcon icon;
    bool isActive = false;
};

class SideBar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int width READ width WRITE setWidth)
    Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed)

public:
    explicit SideBar(QWidget* parent = nullptr);
    ~SideBar() override = default;

    void addItem(const SideBarItem& item);
    void addItems(const QList<SideBarItem>& items);
    void removeItem(const QString& id);
    void clearItems();

    void setActiveItem(const QString& id);
    QString activeItem() const;

    void setWidth(int width);
    int width() const;

    void setCollapsed(bool collapsed);
    bool isCollapsed() const;

    void setItemIconColor(const QString& id, const QColor& color);

signals:
    void itemClicked(const QString& id);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void updateStyle();
    QWidget* createItemWidget(const SideBarItem& item);

    QVBoxLayout* m_layout;
    QList<SideBarItem> m_items;
    QString m_activeId;
    int m_width;
    bool m_collapsed;
};

}

#endif