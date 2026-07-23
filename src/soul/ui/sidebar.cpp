#include "soul/ui/sidebar.h"
#include "soul/ui/sidebar_hover_filter.h"
#include "soul/ui/animation.h"
#include "soul/ui/theme.h"
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

namespace sc {

SideBar::SideBar(QWidget* parent)
    : QWidget(parent),
      m_width(200),
      m_collapsed(false) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 16, 0, 16);
    m_layout->setSpacing(4);
    m_layout->addStretch();

    setFixedWidth(m_width);
}

void SideBar::addItem(const SideBarItem& item) {
    m_items.append(item);
    QWidget* widget = createItemWidget(item);
    m_layout->insertWidget(m_layout->count() - 1, widget);
    updateStyle();
}

void SideBar::addItems(const QList<SideBarItem>& items) {
    for (const auto& item : items) {
        addItem(item);
    }
}

void SideBar::removeItem(const QString& id) {
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id == id) {
            m_items.removeAt(i);
            break;
        }
    }
    clearItems();
    for (const auto& item : m_items) {
        addItem(item);
    }
}

void SideBar::clearItems() {
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_layout->addStretch();
}

void SideBar::setActiveItem(const QString& id) {
    m_activeId = id;
    for (int i = 0; i < m_items.size(); ++i) {
        m_items[i].isActive = (m_items[i].id == id);
    }
    updateStyle();
}

QString SideBar::activeItem() const {
    return m_activeId;
}

void SideBar::setWidth(int width) {
    m_width = width;
    setFixedWidth(width);
    update();
}

int SideBar::width() const {
    return m_width;
}

void SideBar::setCollapsed(bool collapsed) {
    m_collapsed = collapsed;
    setWidth(collapsed ? 60 : 200);
    updateStyle();
}

bool SideBar::isCollapsed() const {
    return m_collapsed;
}

void SideBar::setItemIconColor(const QString& id, const QColor& color) {
    Q_UNUSED(id);
    Q_UNUSED(color);
}

void SideBar::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QWidget* parent = parentWidget();
    if (parent) {
        QPoint pos = mapToParent(rect().topLeft());
        QSize size = rect().size();
        QPixmap source = parent->grab(QRect(pos, size));

        QGraphicsBlurEffect* blurEffect = new QGraphicsBlurEffect();
        blurEffect->setBlurRadius(15);

        QGraphicsScene scene;
        QGraphicsPixmapItem* item = scene.addPixmap(source);
        item->setGraphicsEffect(blurEffect);

        QPixmap blurredBackground(size);
        blurredBackground.fill(Qt::transparent);
        QPainter bgPainter(&blurredBackground);
        scene.render(&bgPainter);

        painter.drawPixmap(rect(), blurredBackground);

        delete blurEffect;
    }

    const Style& s = Theme::instance().style();
    QColor tintColor = s.color(ColorRole::Surface);
    tintColor.setAlpha(190);
    painter.fillRect(rect(), tintColor);

    QLinearGradient gradient(rect().right() - 2, 0, rect().right(), 0);
    gradient.setColorAt(0, s.color(ColorRole::Primary).lighter(150));
    gradient.setColorAt(1, Qt::transparent);
    painter.fillRect(QRect(rect().right() - 2, 0, 2, rect().height()), gradient);
}

void SideBar::updateStyle() {
    for (int i = 0; i < m_layout->count() - 1; ++i) {
        QWidget* widget = m_layout->itemAt(i)->widget();
        if (!widget) continue;

        QPushButton* btn = qobject_cast<QPushButton*>(widget);
        if (!btn) continue;

        QString itemId = btn->property("_item_id").toString();
        bool isActive = (itemId == m_activeId);

        const Style& s = Theme::instance().style();
        QString borderRadius = QString::number(s.cornerRadius(CornerRadius::Medium));
        QString padding = QString::number(s.spacing(Spacing::Small));

        QString styleSheet;
        if (isActive) {
            styleSheet = QString(R"(
                QPushButton {
                    background-color: rgba(99, 102, 241, 0.15);
                    color: %1;
                    border: none;
                    border-radius: %2px;
                    padding: %3px;
                    text-align: left;
                }
                QPushButton:hover {
                    background-color: rgba(99, 102, 241, 0.2);
                }
            )").arg(s.color(ColorRole::Primary).name())
              .arg(borderRadius)
              .arg(padding);
        } else {
            styleSheet = QString(R"(
                QPushButton {
                    background-color: rgba(0,0,0,0);
                    color: %1;
                    border: none;
                    border-radius: %2px;
                    padding: %3px;
                    text-align: left;
                }
                QPushButton:hover {
                    background-color: rgba(99, 102, 241, 0.08);
                }
            )").arg(s.color(ColorRole::TextPrimary).name())
              .arg(borderRadius)
              .arg(padding);
        }

        btn->setStyleSheet(styleSheet);
        btn->setIconSize(QSize(20, 20));
    }
}

QWidget* SideBar::createItemWidget(const SideBarItem& item) {
    QPushButton* btn = new QPushButton(item.text);
    btn->setIcon(item.icon);
    btn->setProperty("_item_id", item.id);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);

    connect(btn, &QPushButton::clicked, this, [this, item]() {
        setActiveItem(item.id);
        emit itemClicked(item.id);
    });

    btn->installEventFilter(new SideBarHoverFilter(btn));

    return btn;
}

}