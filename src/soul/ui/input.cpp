#include "soul/ui/input.h"
#include <QPainter>

namespace sc {

Input::Input(QWidget* parent) : QLineEdit(parent), m_type(Normal), m_hasError(false), m_isFocused(false) {
    init();
}

Input::Input(const QString& placeholder, QWidget* parent)
    : QLineEdit(parent), m_type(Normal), m_hasError(false), m_isFocused(false) {
    setPlaceholderText(placeholder);
    init();
}

void Input::init() {
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFixedHeight(40);
    setContentsMargins(12, 0, 12, 0);
}

Input::InputType Input::inputType() const {
    return m_type;
}

void Input::setInputType(InputType type) {
    m_type = type;
    switch (type) {
    case Password:
        setEchoMode(QLineEdit::Password);
        break;
    case Search:
        setClearButtonEnabled(true);
        break;
    case Email:
        setInputMask("");
        break;
    default:
        setEchoMode(QLineEdit::Normal);
        break;
    }
}

bool Input::hasError() const {
    return m_hasError;
}

void Input::setError(bool error) {
    m_hasError = error;
    update();
}

QString Input::errorMessage() const {
    return m_errorMessage;
}

void Input::setErrorMessage(const QString& message) {
    m_errorMessage = message;
    m_hasError = !message.isEmpty();
    update();
}

void Input::focusInEvent(QFocusEvent* event) {
    m_isFocused = true;
    QLineEdit::focusInEvent(event);
    Animation::applyGlow(this, Theme::instance().style().color(ColorRole::Primary), 150);
}

void Input::focusOutEvent(QFocusEvent* event) {
    m_isFocused = false;
    QLineEdit::focusOutEvent(event);
    Animation::removeGlow(this);
}

void Input::paintEvent(QPaintEvent* event) {
    QLineEdit::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int borderWidth = 2;
    QRect rect = this->rect().adjusted(borderWidth/2, borderWidth/2, -borderWidth/2, -borderWidth/2);
    int radius = Theme::instance().style().cornerRadius(sc::CornerRadius::Small);

    if (m_isFocused && !m_hasError) {
        QColor glowColor = Theme::instance().style().color(ColorRole::Primary);
        glowColor.setAlpha(100);
        QPen pen(glowColor, borderWidth);
        painter.setPen(pen);
        painter.drawRoundedRect(rect, radius, radius);
    } else if (m_hasError) {
        QColor errorColor = Theme::instance().style().color(ColorRole::Error);
        QPen pen(errorColor, borderWidth);
        painter.setPen(pen);
        painter.drawRoundedRect(rect, radius, radius);
    }
}

}
