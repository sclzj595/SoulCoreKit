#include "soul/ui/slider.h"
#include <QPainter>

namespace sc {

Slider::Slider(QWidget* parent)
    : QSlider(parent), m_glowProgress(0.0), m_isDragging(false) {
    m_glowAnimation = new QPropertyAnimation(this, "glowProgress", this);
    m_glowAnimation->setDuration(200);
    m_glowAnimation->setEasingCurve(QEasingCurve::OutQuad);
}

Slider::Slider(Qt::Orientation orientation, QWidget* parent)
    : QSlider(orientation, parent), m_glowProgress(0.0), m_isDragging(false) {
    m_glowAnimation = new QPropertyAnimation(this, "glowProgress", this);
    m_glowAnimation->setDuration(200);
    m_glowAnimation->setEasingCurve(QEasingCurve::OutQuad);
}

qreal Slider::glowProgress() const {
    return m_glowProgress;
}

void Slider::setGlowProgress(qreal progress) {
    m_glowProgress = qBound(0.0, progress, 1.0);
    update();
}

void Slider::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (orientation() == Qt::Horizontal) {
        int height = this->height();
        int width = this->width();
        int trackHeight = 6;
        int trackY = (height - trackHeight) / 2;
        int trackWidth = width - 20;
        int trackX = 10;

        QRect trackRect(trackX, trackY, trackWidth, trackHeight);
        int trackRadius = trackHeight / 2;

        QColor trackColor = Theme::instance().style().color(ColorRole::SurfaceVariant);
        painter.setBrush(trackColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(trackRect, trackRadius, trackRadius);

        int value = this->value();
        int min = this->minimum();
        int max = this->maximum();
        qreal progress = max > min ? (value - min) / static_cast<qreal>(max - min) : 0;

        int filledWidth = static_cast<int>(trackWidth * progress);
        QRect filledRect(trackX, trackY, filledWidth, trackHeight);

        QLinearGradient gradient(filledRect.topLeft(), filledRect.topRight());
        gradient.setColorAt(0, Theme::instance().style().color(ColorRole::Primary));
        gradient.setColorAt(1, Theme::instance().style().color(ColorRole::PrimaryLight));
        painter.setBrush(gradient);
        painter.drawRoundedRect(filledRect, trackRadius, trackRadius);

        if (m_glowProgress > 0) {
            QColor glowColor = Theme::instance().style().color(ColorRole::Primary);
            glowColor.setAlpha(static_cast<int>(m_glowProgress * 100));
            painter.setBrush(glowColor);
            painter.drawRoundedRect(filledRect.adjusted(-4, -4, 4, 4), trackRadius + 4, trackRadius + 4);
        }

        int handleSize = 16;
        int handleX = trackX + filledWidth - handleSize / 2;
        int handleY = (height - handleSize) / 2;
        QRect handleRect(handleX, handleY, handleSize, handleSize);
        int handleRadius = handleSize / 2;

        QRadialGradient handleGradient(handleRect.center(), handleRadius);
        handleGradient.setColorAt(0, QColor("#ffffff"));
        handleGradient.setColorAt(1, Theme::instance().style().color(ColorRole::PrimaryLight));
        painter.setBrush(handleGradient);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(handleRect, handleRadius, handleRadius);

        if (m_isDragging || m_glowProgress > 0) {
            QColor handleGlow = Theme::instance().style().color(ColorRole::Primary);
            handleGlow.setAlpha(static_cast<int>(m_glowProgress * 150));
            painter.setBrush(handleGlow);
            painter.drawRoundedRect(handleRect.adjusted(-6, -6, 6, 6), handleRadius + 6, handleRadius + 6);
        }
    } else {
        int height = this->height();
        int width = this->width();
        int trackWidth = 6;
        int trackX = (width - trackWidth) / 2;
        int trackHeight = height - 20;
        int trackY = 10;

        QRect trackRect(trackX, trackY, trackWidth, trackHeight);
        int trackRadius = trackWidth / 2;

        QColor trackColor = Theme::instance().style().color(ColorRole::SurfaceVariant);
        painter.setBrush(trackColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(trackRect, trackRadius, trackRadius);

        int value = this->value();
        int min = this->minimum();
        int max = this->maximum();
        qreal progress = max > min ? (value - min) / static_cast<qreal>(max - min) : 0;

        int filledHeight = static_cast<int>(trackHeight * progress);
        QRect filledRect(trackX, trackY + trackHeight - filledHeight, trackWidth, filledHeight);

        QLinearGradient gradient(filledRect.bottomLeft(), filledRect.topLeft());
        gradient.setColorAt(0, Theme::instance().style().color(ColorRole::Primary));
        gradient.setColorAt(1, Theme::instance().style().color(ColorRole::PrimaryLight));
        painter.setBrush(gradient);
        painter.drawRoundedRect(filledRect, trackRadius, trackRadius);

        int handleSize = 16;
        int handleX = (width - handleSize) / 2;
        int handleY = trackY + trackHeight - filledHeight - handleSize / 2;
        QRect handleRect(handleX, handleY, handleSize, handleSize);
        int handleRadius = handleSize / 2;

        QRadialGradient handleGradient(handleRect.center(), handleRadius);
        handleGradient.setColorAt(0, QColor("#ffffff"));
        handleGradient.setColorAt(1, Theme::instance().style().color(ColorRole::PrimaryLight));
        painter.setBrush(handleGradient);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(handleRect, handleRadius, handleRadius);
    }
}

void Slider::mousePressEvent(QMouseEvent* event) {
    m_isDragging = true;
    m_glowAnimation->stop();
    m_glowAnimation->setStartValue(m_glowProgress);
    m_glowAnimation->setEndValue(1.0);
    m_glowAnimation->start();
    QSlider::mousePressEvent(event);
}

void Slider::mouseMoveEvent(QMouseEvent* event) {
    QSlider::mouseMoveEvent(event);
}

void Slider::mouseReleaseEvent(QMouseEvent* event) {
    m_isDragging = false;
    m_glowAnimation->stop();
    m_glowAnimation->setStartValue(m_glowProgress);
    m_glowAnimation->setEndValue(0.0);
    m_glowAnimation->start();
    QSlider::mouseReleaseEvent(event);
}

}
