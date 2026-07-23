#include "soul/ui/tool_tip.h"
#include <QPainter>
#include <QPainterPath>

namespace sc {

ToolTip::ToolTip(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
}

QString ToolTip::text() const {
    return m_text;
}

void ToolTip::setText(const QString& text) {
    m_text = text;
    update();
}

void ToolTip::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const Style& s = Theme::instance().style();
    QColor bgColor = s.color(ColorRole::Surface);
    bgColor.setAlpha(240);

    int padding = s.spacing(Spacing::Small);
    int radius = s.cornerRadius(CornerRadius::Medium);

    QRect bgRect = rect().adjusted(padding/2, padding/2, -padding/2, -padding/2);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(bgRect, radius, radius);

    QColor shadowColor = s.color(ColorRole::Shadow);
    shadowColor.setAlpha(50);
    QPainterPath shadowPath;
    shadowPath.addRoundedRect(bgRect.adjusted(2, 2, 2, 2), radius, radius);
    painter.setBrush(shadowColor);
    painter.drawPath(shadowPath);

    painter.setPen(s.color(ColorRole::TextPrimary));
    painter.setFont(s.font());
    painter.drawText(bgRect, Qt::AlignCenter, m_text);
}

QSize ToolTip::sizeHint() const {
    const Style& s = Theme::instance().style();
    int padding = s.spacing(Spacing::Medium);
    QFontMetrics fm(s.font());
    int width = fm.horizontalAdvance(m_text) + padding * 2;
    int height = fm.height() + padding * 2;
    return QSize(width, height);
}

}
