#include "soul/ui/glass_widget.h"
#include <QApplication>
#include <QScreen>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>
#include <QDebug>

namespace sc {
namespace ui {

GlassWidget::GlassWidget(QWidget* parent)
    : QWidget(parent),
      m_blurRadius(15),
      m_opacity(0.65),
      m_tintColor(QColor(255, 255, 255, 166)) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
}

void GlassWidget::setBlurRadius(int radius) {
    m_blurRadius = radius;
    qDebug() << "GlassWidget::setBlurRadius" << radius;
    updateBackground();
    qDebug() << "GlassWidget::setBlurRadius done";
}

int GlassWidget::blurRadius() const {
    return m_blurRadius;
}

void GlassWidget::setOpacity(qreal opacity) {
    m_opacity = opacity;
    update();
}

qreal GlassWidget::opacity() const {
    return m_opacity;
}

void GlassWidget::setTintColor(const QColor& color) {
    m_tintColor = color;
    update();
}

QColor GlassWidget::tintColor() const {
    return m_tintColor;
}

void GlassWidget::update() {
    updateBackground();
    QWidget::update();
}

void GlassWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_blurredBackground.isNull()) {
        painter.drawPixmap(rect(), m_blurredBackground);
    }

    painter.fillRect(rect(), m_tintColor);
}

void GlassWidget::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event)
    updateBackground();
}

void GlassWidget::moveEvent(QMoveEvent* event) {
    Q_UNUSED(event)
    updateBackground();
}

void GlassWidget::updateBackground() {
    if (m_updating) {
        return;
    }
    m_updating = true;

    if (!parentWidget()) {
        m_updating = false;
        return;
    }

    QWidget* parent = parentWidget();
    if (!parent->isVisible() || parent->size().isEmpty()) {
        m_updating = false;
        return;
    }

    QPoint pos = mapToParent(rect().topLeft());
    QSize size = rect().size();

    if (size.isEmpty()) {
        m_updating = false;
        return;
    }

    // Clamp the grab rectangle to the parent's visible area
    QRect grabRect(pos, size);
    QRect parentRect(QPoint(0, 0), parent->size());
    grabRect = grabRect.intersected(parentRect);
    if (grabRect.isEmpty()) {
        m_updating = false;
        return;
    }

    QPixmap source = parent->grab(grabRect);

    QGraphicsScene scene;
    QGraphicsPixmapItem* item = scene.addPixmap(source);

    auto* blurEffect = new QGraphicsBlurEffect();
    blurEffect->setBlurRadius(m_blurRadius);
    item->setGraphicsEffect(blurEffect);

    m_blurredBackground = QPixmap(size);
    m_blurredBackground.fill(Qt::transparent);
    QPainter painter(&m_blurredBackground);
    scene.render(&painter);

    m_updating = false;
}

} // namespace ui
} // namespace sc