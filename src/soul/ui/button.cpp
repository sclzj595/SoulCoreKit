#include "soul/ui/button.h"
#include "soul/ui/animation.h"
#include "soul/ui/theme.h"
#include "soul/ui/design_constants.h"

namespace sc {

Button::Button(QWidget* parent)
    : QPushButton(parent),
      m_type(ButtonType::Push),
      m_size(ButtonSize::Medium),
      m_breathing(false) {
    init();
}

Button::Button(const QString& text, QWidget* parent)
    : QPushButton(text, parent),
      m_type(ButtonType::Push),
      m_size(ButtonSize::Medium),
      m_breathing(false) {
    init();
}

Button::Button(const QIcon& icon, const QString& text, QWidget* parent)
    : QPushButton(icon, text, parent),
      m_type(ButtonType::Push),
      m_size(ButtonSize::Medium),
      m_breathing(false) {
    init();
}

void Button::init() {
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    updateStyle();
}

ButtonType Button::buttonType() const {
    return m_type;
}

void Button::setButtonType(ButtonType type) {
    m_type = type;
    updateStyle();
}

ButtonSize Button::buttonSize() const {
    return m_size;
}

void Button::setButtonSize(ButtonSize size) {
    m_size = size;
    updateStyle();
}

bool Button::isBreathing() const {
    return m_breathing;
}

void Button::setBreathing(bool enabled) {
    m_breathing = enabled;
    updateStyle();
}

void Button::setIconColor(const QColor& color) {
    m_iconColor = color;
    updateStyle();
}

QColor Button::iconColor() const {
    return m_iconColor;
}

void Button::enterEvent(QEnterEvent* event) {
    QPushButton::enterEvent(event);
    Animation::applyGlow(this, Theme::instance().style().color(ColorRole::Primary));
    if (m_breathing) {
        Animation::applyBreathing(this);
    }
}

void Button::leaveEvent(QEvent* event) {
    QPushButton::leaveEvent(event);
    Animation::removeGlow(this);
    Animation::stopBreathing(this);
}

void Button::mousePressEvent(QMouseEvent* event) {
    QPushButton::mousePressEvent(event);
    Animation::applyPress(this);
}

void Button::mouseReleaseEvent(QMouseEvent* event) {
    QPushButton::mouseReleaseEvent(event);
}

void Button::updateStyle() {
    QString styleSheet;
    const Style& s = Theme::instance().style();

    int padding = design::BUTTON_MEDIUM_PADDING;
    int fontSize = design::BUTTON_MEDIUM_FONT_SIZE;
    int iconSize = design::BUTTON_MEDIUM_ICON_SIZE;

    switch (m_size) {
        case ButtonSize::Small:
            padding = design::BUTTON_SMALL_PADDING;
            fontSize = design::BUTTON_SMALL_FONT_SIZE;
            iconSize = design::BUTTON_SMALL_ICON_SIZE;
            break;
        case ButtonSize::Medium:
            padding = design::BUTTON_MEDIUM_PADDING;
            fontSize = design::BUTTON_MEDIUM_FONT_SIZE;
            iconSize = design::BUTTON_MEDIUM_ICON_SIZE;
            break;
        case ButtonSize::Large:
            padding = design::BUTTON_LARGE_PADDING;
            fontSize = design::BUTTON_LARGE_FONT_SIZE;
            iconSize = design::BUTTON_LARGE_ICON_SIZE;
            break;
    }

    QString borderRadius = QString::number(s.cornerRadius(CornerRadius::Medium));

    QColor primary = s.color(ColorRole::Primary);
    QString primaryWithAlpha10 = QString("rgba(%1,%2,%3,0.1)")
        .arg(primary.red()).arg(primary.green()).arg(primary.blue());
    QString primaryWithAlpha08 = QString("rgba(%1,%2,%3,0.08)")
        .arg(primary.red()).arg(primary.green()).arg(primary.blue());
    QString primaryWithAlpha05 = QString("rgba(%1,%2,%3,0.05)")
        .arg(primary.red()).arg(primary.green()).arg(primary.blue());

    switch (m_type) {
        case ButtonType::Push:
            styleSheet = QString(R"(
                QPushButton {
                    background-color: %1;
                    color: %2;
                    border: none;
                    border-radius: %3px;
                    padding: %4px %5px;
                    font-size: %6px;
                    font-weight: 500;
                    min-width: 80px;
                }
                QPushButton:hover {
                    background-color: %7;
                }
                QPushButton:pressed {
                    background-color: %8;
                }
            )").arg(s.color(ColorRole::Primary).name())
              .arg(s.color(ColorRole::OnPrimary).name())
              .arg(borderRadius)
              .arg(padding)
              .arg(padding * 2)
              .arg(fontSize)
              .arg(s.color(ColorRole::PrimaryLight).name())
              .arg(s.color(ColorRole::PrimaryDark).name());
            break;

        case ButtonType::Icon:
            styleSheet = QString(R"(
                QPushButton {
                    background-color: rgba(0,0,0,0);
                    color: %1;
                    border: none;
                    border-radius: %2px;
                    padding: %3px;
                    min-width: %4px;
                    min-height: %4px;
                }
                QPushButton:hover {
                    background-color: %5;
                }
            )").arg(s.color(ColorRole::TextPrimary).name())
              .arg(borderRadius)
              .arg(padding)
              .arg(padding * 3)
              .arg(primaryWithAlpha10);
            break;

        case ButtonType::Flat:
            styleSheet = QString(R"(
                QPushButton {
                    background-color: rgba(0,0,0,0);
                    color: %1;
                    border: none;
                    border-radius: %2px;
                    padding: %3px %4px;
                    font-size: %5px;
                    min-width: 80px;
                }
                QPushButton:hover {
                    background-color: %6;
                }
            )").arg(s.color(ColorRole::TextPrimary).name())
              .arg(borderRadius)
              .arg(padding)
              .arg(padding * 2)
              .arg(fontSize)
              .arg(primaryWithAlpha08);
            break;

        case ButtonType::Outline:
            styleSheet = QString(R"(
                QPushButton {
                    background-color: rgba(0,0,0,0);
                    color: %1;
                    border: 1px solid %2;
                    border-radius: %3px;
                    padding: %4px %5px;
                    font-size: %6px;
                    font-weight: 500;
                    min-width: 80px;
                }
                QPushButton:hover {
                    border-color: %7;
                    background-color: %8;
                }
            )").arg(s.color(ColorRole::Primary).name())
              .arg(s.color(ColorRole::Border).name())
              .arg(borderRadius)
              .arg(padding)
              .arg(padding * 2)
              .arg(fontSize)
              .arg(s.color(ColorRole::Primary).name())
              .arg(primaryWithAlpha05);
            break;
    }

    setStyleSheet(styleSheet);
    setIconSize(QSize(iconSize, iconSize));
}

}