#ifndef SOUL_UI_SLIDER_H
#define SOUL_UI_SLIDER_H

#include "soul/ui/theme.h"
#include <QSlider>
#include <QPropertyAnimation>

namespace sc {

class Slider : public QSlider {
    Q_OBJECT
    Q_PROPERTY(qreal glowProgress READ glowProgress WRITE setGlowProgress)

public:
    explicit Slider(QWidget* parent = nullptr);
    explicit Slider(Qt::Orientation orientation, QWidget* parent = nullptr);

    qreal glowProgress() const;
    void setGlowProgress(qreal progress);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    qreal m_glowProgress;
    bool m_isDragging;
    QPropertyAnimation* m_glowAnimation;
};

}

#endif