#include "soul/ui/checkbox.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>

namespace sc {

Checkbox::Checkbox(QWidget* parent)
    : QAbstractButton(parent), m_checked(false), m_checkProgress(0.0) {
    setCheckable(true);
    m_animation = new QPropertyAnimation(this, "checkProgress", this);
    m_animation->setDuration(200);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);
}

Checkbox::Checkbox(const QString& text, QWidget* parent)
    : QAbstractButton(parent), m_checked(false), m_checkProgress(0.0) {
    setText(text);
    setCheckable(true);
    m_animation = new QPropertyAnimation(this, "checkProgress", this);
    m_animation->setDuration(200);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);
}

bool Checkbox::isChecked() const {
    return m_checked;
}

void Checkbox::setChecked(bool checked) {
    if (m_checked == checked) return;
    m_checked = checked;

    m_animation->stop();
    m_animation->setStartValue(m_checkProgress);
    m_animation->setEndValue(checked ? 1.0 : 0.0);
    m_animation->start();

    Q_EMIT toggled(checked);
}

qreal Checkbox::checkProgress() const {
    return m_checkProgress;
}

void Checkbox::setCheckProgress(qreal progress) {
    m_checkProgress = qBound(0.0, progress, 1.0);
    update();
}

void Checkbox::mouseReleaseEvent(QMouseEvent* event) {
    if (rect().contains(event->pos())) {
        setChecked(!m_checked);
    }
    QAbstractButton::mouseReleaseEvent(event);
}

void Checkbox::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int checkSize = 20;
    int margin = 4;
    QRect checkRect(margin, margin, checkSize, checkSize);
    int radius = Theme::instance().style().cornerRadius(CornerRadius::Small);

    QColor borderColor = m_checked ? Theme::instance().style().color(ColorRole::Primary)
                                   : Theme::instance().style().color(ColorRole::Border);
    QColor fillColor = Theme::instance().style().color(ColorRole::Primary);
    fillColor.setAlpha(static_cast<int>(m_checkProgress * 255));

    painter.setPen(QPen(borderColor, 2));
    painter.setBrush(fillColor);
    painter.drawRoundedRect(checkRect, radius, radius);

    if (m_checkProgress > 0) {
        QColor checkColor = Theme::instance().style().color(ColorRole::OnPrimary);
        QPen pen(checkColor, 2);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        QPainterPath path;
        QPoint start(checkRect.left() + checkSize * 0.25, checkRect.top() + checkSize * 0.5);
        QPoint mid(checkRect.left() + checkSize * 0.45, checkRect.top() + checkSize * 0.7);
        QPoint end(checkRect.right() - checkSize * 0.2, checkRect.top() + checkSize * 0.35);

        path.moveTo(start);
        path.lineTo(mid);
        path.lineTo(end);

        painter.save();
        QRect clipRect = checkRect.adjusted(-2, -2, 2, 2);
        clipRect.setWidth(clipRect.width() * m_checkProgress);
        painter.setClipRect(clipRect);
        painter.drawPath(path);
        painter.restore();
    }

    if (!text().isEmpty()) {
        int textX = checkSize + margin * 3;
        int textY = margin;
        QRect textRect(textX, textY, width() - textX - margin, height());
        painter.setPen(Theme::instance().style().color(ColorRole::TextPrimary));
        painter.setFont(Theme::instance().style().font());
        painter.drawText(textRect, Qt::AlignVCenter, text());
    }
}

QSize Checkbox::sizeHint() const {
    int checkSize = 20;
    int margin = 4;
    int textWidth = fontMetrics().horizontalAdvance(text());
    return QSize(checkSize + textWidth + margin * 4, checkSize + margin * 2);
}

}
