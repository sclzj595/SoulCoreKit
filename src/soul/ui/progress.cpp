#include "soul/ui/progress.h"
#include <QPainter>

namespace sc {

Progress::Progress(QWidget* parent)
    : QWidget(parent), m_min(0), m_max(100), m_value(0), m_glowProgress(0.0) {
    m_glowAnimation = new QPropertyAnimation(this, "glowProgress", this);
    m_glowAnimation->setDuration(1000);
    m_glowAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    m_glowAnimation->setLoopCount(-1);
    m_glowAnimation->setStartValue(0.3);
    m_glowAnimation->setEndValue(1.0);
    m_glowAnimation->start();

    setFixedHeight(8);
}

int Progress::value() const {
    return m_value;
}

void Progress::setValue(int value) {
    m_value = qBound(m_min, value, m_max);
    update();
}

int Progress::minimum() const {
    return m_min;
}

void Progress::setMinimum(int min) {
    m_min = min;
    if (m_value < m_min) m_value = m_min;
    update();
}

int Progress::maximum() const {
    return m_max;
}

void Progress::setMaximum(int max) {
    m_max = max;
    if (m_value > m_max) m_value = m_max;
    update();
}

qreal Progress::glowProgress() const {
    return m_glowProgress;
}

void Progress::setGlowProgress(qreal progress) {
    m_glowProgress = qBound(0.0, progress, 1.0);
    update();
}

void Progress::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const Style& s = Theme::instance().style();
    int height = this->height();
    int radius = height / 2;

    QColor trackColor = s.color(ColorRole::SurfaceVariant);
    trackColor.setAlpha(60);
    painter.setBrush(trackColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), radius, radius);

    int range = m_max - m_min;
    if (range > 0) {
        qreal progress = (m_value - m_min) / static_cast<qreal>(range);
        int filledWidth = static_cast<int>(width() * progress);

        QRect filledRect(0, 0, filledWidth, height);

        QLinearGradient gradient(filledRect.topLeft(), filledRect.topRight());
        gradient.setColorAt(0, s.color(ColorRole::Primary));
        gradient.setColorAt(1, s.color(ColorRole::PrimaryLight));
        painter.setBrush(gradient);
        painter.drawRoundedRect(filledRect, radius, radius);

        QColor glowColor = s.color(ColorRole::Primary);
        glowColor.setAlpha(static_cast<int>(m_glowProgress * 80));
        painter.setBrush(glowColor);
        painter.drawRoundedRect(filledRect.adjusted(-2, -2, 2, 2), radius + 2, radius + 2);
    }
}

}
