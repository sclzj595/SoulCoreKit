#include "soul/ui/switch.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QMouseEvent>

namespace sc {

Switch::Switch(QWidget* parent)
    : QAbstractButton(parent), m_checked(false), m_sliderPosition(0.0) {
    setCheckable(true);
    m_animation = new QPropertyAnimation(this, "sliderPosition", this);
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);
}

bool Switch::isChecked() const {
    return m_checked;
}

void Switch::setChecked(bool checked) {
    if (m_checked == checked) return;
    m_checked = checked;

    m_animation->stop();
    m_animation->setStartValue(m_sliderPosition);
    m_animation->setEndValue(checked ? 1.0 : 0.0);
    m_animation->start();

    Q_EMIT toggled(checked);
}

qreal Switch::sliderPosition() const {
    return m_sliderPosition;
}

void Switch::setSliderPosition(qreal position) {
    m_sliderPosition = qBound(0.0, position, 1.0);
    update();
}

void Switch::mouseReleaseEvent(QMouseEvent* event) {
    if (rect().contains(event->pos())) {
        setChecked(!m_checked);
    }
    QAbstractButton::mouseReleaseEvent(event);
}

void Switch::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QSize size = this->size();
    int height = size.height();
    int width = size.width();
    int radius = height / 2;
    int sliderSize = height - 6;
    int sliderRadius = sliderSize / 2;

    QRect backgroundRect(0, 0, width, height);
    QColor bgColor = m_checked ? Theme::instance().style().color(ColorRole::Primary)
                               : Theme::instance().style().color(ColorRole::SurfaceVariant);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(backgroundRect, radius, radius);

    int sliderX = 3 + m_sliderPosition * (width - sliderSize - 6);
    int sliderY = 3;
    QRect sliderRect(sliderX, sliderY, sliderSize, sliderSize);

    QRadialGradient gradient(sliderRect.center(), sliderRadius);
    gradient.setColorAt(0, QColor("#ffffff"));
    gradient.setColorAt(1, QColor("#e2e8f0"));
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(sliderRect, sliderRadius, sliderRadius);

    if (m_checked) {
        QColor glowColor = Theme::instance().style().color(ColorRole::Primary);
        glowColor.setAlpha(50);
        painter.setBrush(glowColor);
        painter.drawRoundedRect(backgroundRect, radius, radius);
    }
}

QSize Switch::sizeHint() const {
    return QSize(52, 28);
}

}
