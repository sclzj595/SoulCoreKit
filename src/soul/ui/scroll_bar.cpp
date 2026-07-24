#include "soul/ui/scroll_bar.h"
#include <QPainter>

namespace sc {

ScrollBar::ScrollBar(QWidget* parent) : QScrollBar(parent), m_isDragging(false) {
    setStyleSheet("QScrollBar { width: 6px; }");
}

ScrollBar::ScrollBar(Qt::Orientation orientation, QWidget* parent)
    : QScrollBar(orientation, parent), m_isDragging(false) {
    if (orientation == Qt::Horizontal) {
        setStyleSheet("QScrollBar { height: 6px; }");
    } else {
        setStyleSheet("QScrollBar { width: 6px; }");
    }
}

void ScrollBar::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const Style& s = Theme::instance().style();
    QColor trackColor = s.color(ColorRole::SurfaceVariant);
    trackColor.setAlpha(80);

    if (orientation() == Qt::Vertical) {
        int trackWidth = 6;
        int trackX = (width() - trackWidth) / 2;
        QRect trackRect(trackX, 0, trackWidth, height());
        painter.setBrush(trackColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(trackRect, trackWidth / 2, trackWidth / 2);

        int min = minimum();
        int max = maximum();
        int value = this->value();
        int range = max - min;

        if (range > 0) {
            int sliderHeight = qMax(20, height() * height() / (height() + range));
            int sliderY = ((height() - sliderHeight) * (value - min)) / range;
            QRect sliderRect(trackX, sliderY, trackWidth, sliderHeight);

            QLinearGradient gradient(sliderRect.topLeft(), sliderRect.bottomLeft());
            gradient.setColorAt(0, s.color(ColorRole::TextSecondary));
            gradient.setColorAt(1, s.color(ColorRole::TextTertiary));
            painter.setBrush(gradient);
            painter.drawRoundedRect(sliderRect, trackWidth / 2, trackWidth / 2);

            if (m_isDragging) {
                QColor glowColor = s.color(ColorRole::Primary);
                glowColor.setAlpha(60);
                painter.setBrush(glowColor);
                painter.drawRoundedRect(sliderRect.adjusted(-4, -4, 4, 4), trackWidth / 2 + 4, trackWidth / 2 + 4);
            }
        }
    } else {
        int trackHeight = 6;
        int trackY = (height() - trackHeight) / 2;
        QRect trackRect(0, trackY, width(), trackHeight);
        painter.setBrush(trackColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(trackRect, trackHeight / 2, trackHeight / 2);

        int min = minimum();
        int max = maximum();
        int value = this->value();
        int range = max - min;

        if (range > 0) {
            int sliderWidth = qMax(20, width() * width() / (width() + range));
            int sliderX = ((width() - sliderWidth) * (value - min)) / range;
            QRect sliderRect(sliderX, trackY, sliderWidth, trackHeight);

            QLinearGradient gradient(sliderRect.topLeft(), sliderRect.topRight());
            gradient.setColorAt(0, s.color(ColorRole::TextSecondary));
            gradient.setColorAt(1, s.color(ColorRole::TextTertiary));
            painter.setBrush(gradient);
            painter.drawRoundedRect(sliderRect, trackHeight / 2, trackHeight / 2);

            if (m_isDragging) {
                QColor glowColor = s.color(ColorRole::Primary);
                glowColor.setAlpha(60);
                painter.setBrush(glowColor);
                painter.drawRoundedRect(sliderRect.adjusted(-4, -4, 4, 4), trackHeight / 2 + 4, trackHeight / 2 + 4);
            }
        }
    }
}

void ScrollBar::mousePressEvent(QMouseEvent* event) {
    m_isDragging = true;
    update();
    QScrollBar::mousePressEvent(event);
}

void ScrollBar::mouseMoveEvent(QMouseEvent* event) {
    QScrollBar::mouseMoveEvent(event);
}

void ScrollBar::mouseReleaseEvent(QMouseEvent* event) {
    m_isDragging = false;
    update();
    QScrollBar::mouseReleaseEvent(event);
}

}
