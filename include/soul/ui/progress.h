#ifndef SOUL_UI_PROGRESS_H
#define SOUL_UI_PROGRESS_H

#include "soul/ui/theme.h"
#include <QWidget>
#include <QPropertyAnimation>

namespace sc {

class Progress : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(qreal glowProgress READ glowProgress WRITE setGlowProgress)

public:
    explicit Progress(QWidget* parent = nullptr);

    int value() const;
    void setValue(int value);

    int minimum() const;
    void setMinimum(int min);

    int maximum() const;
    void setMaximum(int max);

    qreal glowProgress() const;
    void setGlowProgress(qreal progress);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_min;
    int m_max;
    int m_value;
    qreal m_glowProgress;
    QPropertyAnimation* m_glowAnimation;
};

}

#endif