#ifndef SOUL_UI_SWITCH_H
#define SOUL_UI_SWITCH_H

#include "soul/ui/theme.h"
#include <QAbstractButton>
#include <QPropertyAnimation>

namespace sc {

class Switch : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
    Q_PROPERTY(qreal sliderPosition READ sliderPosition WRITE setSliderPosition)

public:
    explicit Switch(QWidget* parent = nullptr);

    bool isChecked() const;
    void setChecked(bool checked);

    qreal sliderPosition() const;
    void setSliderPosition(qreal position);

Q_SIGNALS:
    void toggled(bool checked);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    bool m_checked;
    qreal m_sliderPosition;
    QPropertyAnimation* m_animation;
};

}

#endif