#ifndef SOUL_UI_ANIMATION_H
#define SOUL_UI_ANIMATION_H

#include <QPropertyAnimation>
#include <QWidget>
#include <QGraphicsDropShadowEffect>
#include "soul/ui/design_constants.h"

namespace sc {

class Animation {
public:
    static void fadeIn(QWidget* widget, int duration = design::ANIMATION_DURATION_LONG);
    static void fadeOut(QWidget* widget, int duration = design::ANIMATION_DURATION_LONG);

    static void slideInFromTop(QWidget* widget, int duration = design::ANIMATION_DURATION_LONG);
    static void slideInFromBottom(QWidget* widget, int duration = design::ANIMATION_DURATION_LONG);
    static void slideInFromLeft(QWidget* widget, int duration = design::ANIMATION_DURATION_LONG);
    static void slideInFromRight(QWidget* widget, int duration = design::ANIMATION_DURATION_LONG);

    static void scaleUp(QWidget* widget, int duration = design::ANIMATION_DURATION_LONG);
    static void scaleDown(QWidget* widget, int duration = design::ANIMATION_DURATION_LONG);

    static void shake(QWidget* widget, int duration = design::ANIMATION_DURATION_LONG);

    static void applyBreathing(QWidget* widget, int duration = design::ANIMATION_DURATION_BREATHING);
    static void stopBreathing(QWidget* widget);

    static void applyGlow(QWidget* widget, const QColor& color, int duration = design::ANIMATION_DURATION_NORMAL);
    static void removeGlow(QWidget* widget);

    static void applyPress(QWidget* widget, int duration = design::ANIMATION_DURATION_SHORT);

    static void applyHoverLift(QWidget* widget, int duration = design::ANIMATION_DURATION_NORMAL);
    static void removeHoverLift(QWidget* widget);

private:
    static void animate(QWidget* widget, const QByteArray& propertyName,
                        const QVariant& startValue, const QVariant& endValue,
                        int duration, QEasingCurve::Type curve = QEasingCurve::OutCubic);

    static QPropertyAnimation* getOrCreateAnimation(QWidget* widget, const QString& key);
};

}

#endif
