#ifndef SOUL_UI_BUTTON_H
#define SOUL_UI_BUTTON_H

#include <QPushButton>
#include <QIcon>
#include <QString>

namespace sc {

enum class ButtonType {
    Push,
    Icon,
    Flat,
    Outline
};

enum class ButtonSize {
    Small,
    Medium,
    Large
};

class Button : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(ButtonType buttonType READ buttonType WRITE setButtonType)
    Q_PROPERTY(ButtonSize buttonSize READ buttonSize WRITE setButtonSize)
    Q_PROPERTY(bool breathing READ isBreathing WRITE setBreathing)

public:
    explicit Button(QWidget* parent = nullptr);
    explicit Button(const QString& text, QWidget* parent = nullptr);
    Button(const QIcon& icon, const QString& text, QWidget* parent = nullptr);
    ~Button() override = default;

    ButtonType buttonType() const;
    void setButtonType(ButtonType type);

    ButtonSize buttonSize() const;
    void setButtonSize(ButtonSize size);

    bool isBreathing() const;
    void setBreathing(bool enabled);

    void setIconColor(const QColor& color);
    QColor iconColor() const;

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void init();
    void updateStyle();

    ButtonType m_type;
    ButtonSize m_size;
    bool m_breathing;
    QColor m_iconColor;
};

}

#endif