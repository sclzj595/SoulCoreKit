#ifndef SOUL_UI_INPUT_H
#define SOUL_UI_INPUT_H

#include "soul/ui/theme.h"
#include "soul/ui/animation.h"
#include <QLineEdit>

namespace sc {

class Input : public QLineEdit {
    Q_OBJECT
public:
    enum InputType {
        Normal,
        Password,
        Search,
        Email
    };

    explicit Input(QWidget* parent = nullptr);
    explicit Input(const QString& placeholder, QWidget* parent = nullptr);

    InputType inputType() const;
    void setInputType(InputType type);

    bool hasError() const;
    void setError(bool error);

    QString errorMessage() const;
    void setErrorMessage(const QString& message);

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void init();

    InputType m_type;
    bool m_hasError;
    QString m_errorMessage;
    bool m_isFocused;
};

}

#endif