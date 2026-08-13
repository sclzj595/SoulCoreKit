#include "soul/ui/spinner.h"
#include <QPainter>

namespace sc {

Spinner::Spinner(QWidget* parent) : QWidget(parent), m_rotation(0.0) {
    m_animation = new QPropertyAnimation(this, "rotation", this);
    m_animation->setDuration(1000);
    m_animation->setEasingCurve(QEasingCurve::Linear);
    m_animation->setLoopCount(-1);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(360.0);
    m_animation->start();

    setFixedSize(32, 32);
}

qreal Spinner::rotation() const {
    return m_rotation;
}

void Spinner::setRotation(qreal rotation) {
    m_rotation = rotation;
    update();
}

void Spinner::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.translate(width() / 2, height() / 2);
    painter.rotate(m_rotation);

    const Style& s = Theme::instance().style();
    int radius = 10;
    int arcLength = 270;

    QPen pen(s.color(ColorRole::Primary), 3);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    painter.drawArc(-radius, -radius, radius * 2, radius * 2, 90 * 16, arcLength * 16);

    QColor glowColor = s.color(ColorRole::Primary);
    glowColor.setAlpha(50);
    QPen glowPen(glowColor, 6);
    glowPen.setCapStyle(Qt::RoundCap);
    painter.setPen(glowPen);
    painter.drawArc(-radius, -radius, radius * 2, radius * 2, 90 * 16, arcLength * 16);
}

}
