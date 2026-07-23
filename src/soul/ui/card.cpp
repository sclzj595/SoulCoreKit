#include "soul/ui/card.h"
#include "soul/ui/animation.h"
#include "soul/ui/theme.h"
#include "soul/ui/design_constants.h"
#include "soul/ui/glass_effect_cache.h"
#include <QPainter>

namespace sc {

Card::Card(QWidget* parent)
    : QWidget(parent),
      m_borderRadius(design::DEFAULT_BORDER_RADIUS),
      m_opacity(design::DEFAULT_OPACITY),
      m_hoverEnabled(true),
      m_blurRadius(design::DEFAULT_BLUR_RADIUS) {
    m_tintColor = Theme::instance().style().color(ColorRole::GlassTint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(design::CARD_CONTENT_MARGIN, design::CARD_CONTENT_MARGIN, 
                                 design::CARD_CONTENT_MARGIN, design::CARD_CONTENT_MARGIN);
    m_layout->setSpacing(design::CARD_CONTENT_SPACING);

    updateStyle();
}

void Card::setBorderRadius(int radius) {
    m_borderRadius = radius;
    update();
}

int Card::borderRadius() const {
    return m_borderRadius;
}

void Card::setOpacity(qreal opacity) {
    m_opacity = opacity;
    update();
}

qreal Card::opacity() const {
    return m_opacity;
}

void Card::setHoverEnabled(bool enabled) {
    m_hoverEnabled = enabled;
    if (!enabled) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::PointingHandCursor);
    }
}

bool Card::isHoverEnabled() const {
    return m_hoverEnabled;
}

void Card::setTintColor(const QColor& color) {
    m_tintColor = color;
    update();
}

QColor Card::tintColor() const {
    return m_tintColor;
}

void Card::setBlurRadius(int radius) {
    m_blurRadius = radius;
    update();
}

int Card::blurRadius() const {
    return m_blurRadius;
}

QLayout* Card::contentLayout() {
    return m_layout;
}

void Card::enterEvent(QEnterEvent* event) {
    QWidget::enterEvent(event);
    if (m_hoverEnabled) {
        Animation::applyGlow(this, Theme::instance().style().color(ColorRole::Primary));
        Animation::applyBreathing(this);
        Animation::applyHoverLift(this);
    }
}

void Card::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    Animation::removeGlow(this);
    Animation::stopBreathing(this);
    Animation::removeHoverLift(this);
}

void Card::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QWidget* parent = parentWidget();
    if (parent) {
        QPoint pos = mapToParent(rect().topLeft());
        QSize size = rect().size();
        QPixmap source = parent->grab(QRect(pos, size));

        QPixmap blurredBackground = GlassEffectCache::instance().blur(source, m_blurRadius);
        painter.drawPixmap(rect(), blurredBackground);
    }

    QPainterPath path;
    path.addRoundedRect(rect(), m_borderRadius, m_borderRadius);

    painter.fillPath(path, m_tintColor);

    const Style& s = Theme::instance().style();
    painter.setPen(QPen(s.color(ColorRole::BorderLight), 1));
    painter.drawPath(path);
}

void Card::updateStyle() {
    if (m_hoverEnabled) {
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

}