#include "soul/ui/badge.h"
#include <QPainter>

namespace sc {

Badge::Badge(QWidget* parent) : QWidget(parent), m_count(0), m_visible(true) {
    setAttribute(Qt::WA_TranslucentBackground);
}

int Badge::count() const {
    return m_count;
}

void Badge::setCount(int count) {
    m_count = count;
    m_visible = (count > 0);
    update();
}

bool Badge::isVisible() const {
    return m_visible && m_count > 0;
}

void Badge::setVisible(bool visible) {
    m_visible = visible;
    update();
}

void Badge::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    if (!m_visible || m_count <= 0) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const Style& s = Theme::instance().style();
    QColor bgColor = s.color(ColorRole::Error);

    QString text = m_count > 99 ? "99+" : QString::number(m_count);
    QFontMetrics fm(s.font());
    int textWidth = fm.horizontalAdvance(text);
    int textHeight = fm.height();

    int padding = s.spacing(Spacing::Tiny);
    int minWidth = qMax(textWidth + padding * 2, textHeight + padding);
    int height = textHeight + padding;

    QRect badgeRect(0, 0, minWidth, height);
    int radius = height / 2;

    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(badgeRect, radius, radius);

    QColor glowColor = bgColor;
    glowColor.setAlpha(60);
    painter.setBrush(glowColor);
    painter.drawRoundedRect(badgeRect.adjusted(-2, -2, 2, 2), radius + 2, radius + 2);

    painter.setPen(s.color(ColorRole::OnError));
    painter.setFont(s.smallFont());
    painter.drawText(badgeRect, Qt::AlignCenter, text);
}

QSize Badge::sizeHint() const {
    if (!m_visible || m_count <= 0) return QSize(0, 0);

    const Style& s = Theme::instance().style();
    QString text = m_count > 99 ? "99+" : QString::number(m_count);
    QFontMetrics fm(s.font());
    int textWidth = fm.horizontalAdvance(text);
    int textHeight = fm.height();

    int padding = s.spacing(Spacing::Tiny);
    int minWidth = qMax(textWidth + padding * 2, textHeight + padding);
    int height = textHeight + padding;

    return QSize(minWidth, height);
}

}
