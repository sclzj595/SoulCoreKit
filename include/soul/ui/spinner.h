#ifndef SOUL_UI_SPINNER_H
#define SOUL_UI_SPINNER_H

#include "soul/ui/theme.h"
#include <QWidget>
#include <QPropertyAnimation>

namespace sc {

class Spinner : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)

public:
    explicit Spinner(QWidget* parent = nullptr);

    qreal rotation() const;
    void setRotation(qreal rotation);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qreal m_rotation;
    QPropertyAnimation* m_animation;
};

}

#endif