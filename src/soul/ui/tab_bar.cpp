#include "soul/ui/tab_bar.h"
#include <QPainter>

namespace sc {

TabBar::TabBar(QWidget* parent) : QTabBar(parent) {
    setExpanding(false);
    setElideMode(Qt::ElideMiddle);
}

void TabBar::paintEvent(QPaintEvent* event) {
    QTabBar::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (count() > 0) {
        int activeIndex = currentIndex();
        QRect activeRect = tabRect(activeIndex);

        const Style& s = Theme::instance().style();
        QColor glowColor = s.color(ColorRole::Primary);
        glowColor.setAlpha(40);
        painter.setBrush(glowColor);
        painter.setPen(Qt::NoPen);

        int radius = s.cornerRadius(CornerRadius::Medium);
        painter.drawRoundedRect(activeRect.adjusted(-4, -2, 4, 2), radius, radius);
    }
}

void TabBar::paintTab(QPainter* painter, int index, const QRect& rect) const {
    Q_UNUSED(index);
    Q_UNUSED(rect);
}

}
