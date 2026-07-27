#include "soul/ui/animation.h"
#include "soul/ui/design_constants.h"
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QMap>
#include <QPointer>
#include <memory>
#include <mutex>

namespace sc {

namespace {
    QMap<QPointer<QWidget>, QMap<QString, QPointer<QPropertyAnimation>>> s_animationCache;
    QMap<QPointer<QWidget>, QPointer<QGraphicsDropShadowEffect>> s_glowEffectCache;
    // TSan-safe: Animation statics are process-global; any thread invoking an
    // Animation method must serialize against this mutex. QWidget/QPropertyAnimation
    // themselves remain subject to Qt thread affinity (must be touched on GUI thread).
    std::mutex s_animationMutex;

    void cleanupWidgetAnimations(QWidget* widget) {
        QPointer<QWidget> wp(widget);
        std::lock_guard<std::mutex> lock(s_animationMutex);
        if (s_animationCache.contains(wp)) {
            for (auto& anim : s_animationCache[wp]) {
                if (anim) {
                    // stop() 会触发 DeleteWhenStopped 自动删除,无需 deleteLater
                    anim->stop();
                }
            }
            s_animationCache.remove(wp);
        }
        s_glowEffectCache.remove(wp);
    }
}

void Animation::fadeIn(QWidget* widget, int duration) {
    widget->show();
    widget->setWindowOpacity(0);
    animate(widget, "windowOpacity", 0, 1, duration);
}

void Animation::fadeOut(QWidget* widget, int duration) {
    animate(widget, "windowOpacity", 1, 0, duration, QEasingCurve::InCubic);
}

void Animation::slideInFromTop(QWidget* widget, int duration) {
    QRect geometry = widget->geometry();
    int startY = geometry.top() - geometry.height();
    widget->move(geometry.left(), startY);
    animate(widget, "y", startY, geometry.top(), duration);
}

void Animation::slideInFromBottom(QWidget* widget, int duration) {
    QRect geometry = widget->geometry();
    int startY = geometry.top() + geometry.height();
    widget->move(geometry.left(), startY);
    animate(widget, "y", startY, geometry.top(), duration);
}

void Animation::slideInFromLeft(QWidget* widget, int duration) {
    QRect geometry = widget->geometry();
    int startX = geometry.left() - geometry.width();
    widget->move(startX, geometry.top());
    animate(widget, "x", startX, geometry.left(), duration);
}

void Animation::slideInFromRight(QWidget* widget, int duration) {
    QRect geometry = widget->geometry();
    int startX = geometry.left() + geometry.width();
    widget->move(startX, geometry.top());
    animate(widget, "x", startX, geometry.left(), duration);
}

void Animation::scaleUp(QWidget* widget, int duration) {
    QRect originalRect = widget->geometry();
    int targetWidth = originalRect.width();
    int targetHeight = originalRect.height();
    int startWidth = targetWidth * design::SCALE_ANIMATION_START_FACTOR;
    int startHeight = targetHeight * design::SCALE_ANIMATION_START_FACTOR;
    
    QRect startRect(originalRect.left() + (targetWidth - startWidth) / 2,
                    originalRect.top() + (targetHeight - startHeight) / 2,
                    startWidth, startHeight);
    
    widget->setGeometry(startRect);
    
    QPropertyAnimation* animation = new QPropertyAnimation(widget, "geometry");
    animation->setDuration(duration);
    animation->setStartValue(startRect);
    animation->setEndValue(originalRect);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Animation::scaleDown(QWidget* widget, int duration) {
    QRect originalRect = widget->geometry();
    int targetWidth = originalRect.width() * design::SCALE_ANIMATION_START_FACTOR;
    int targetHeight = originalRect.height() * design::SCALE_ANIMATION_START_FACTOR;
    
    QRect targetRect(originalRect.left() + (originalRect.width() - targetWidth) / 2,
                     originalRect.top() + (originalRect.height() - targetHeight) / 2,
                     targetWidth, targetHeight);
    
    QPropertyAnimation* animation = new QPropertyAnimation(widget, "geometry");
    animation->setDuration(duration);
    animation->setStartValue(originalRect);
    animation->setEndValue(targetRect);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Animation::shake(QWidget* widget, int duration) {
    QPropertyAnimation* animation = new QPropertyAnimation(widget, "pos");
    animation->setDuration(duration);
    animation->setKeyValueAt(0, widget->pos());
    animation->setKeyValueAt(0.1, widget->pos() + QPoint(-design::SHAKE_OFFSET, 0));
    animation->setKeyValueAt(0.2, widget->pos() + QPoint(design::SHAKE_OFFSET, 0));
    animation->setKeyValueAt(0.3, widget->pos() + QPoint(-design::SHAKE_OFFSET, 0));
    animation->setKeyValueAt(0.4, widget->pos() + QPoint(design::SHAKE_OFFSET, 0));
    animation->setKeyValueAt(0.5, widget->pos() + QPoint(-design::SHAKE_OFFSET, 0));
    animation->setKeyValueAt(0.6, widget->pos() + QPoint(design::SHAKE_OFFSET, 0));
    animation->setKeyValueAt(0.7, widget->pos() + QPoint(-design::SHAKE_OFFSET, 0));
    animation->setKeyValueAt(0.8, widget->pos() + QPoint(design::SHAKE_OFFSET, 0));
    animation->setKeyValueAt(0.9, widget->pos() + QPoint(-design::SHAKE_END_OFFSET, 0));
    animation->setKeyValueAt(1, widget->pos());
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Animation::applyBreathing(QWidget* widget, int duration) {
    stopBreathing(widget);

    QRect originalRect = widget->geometry();
    int targetWidth = originalRect.width() * design::BREATHING_SCALE_FACTOR;
    int targetHeight = originalRect.height() * design::BREATHING_SCALE_FACTOR;

    QRect targetRect(originalRect.left() + (originalRect.width() - targetWidth) / 2,
                     originalRect.top() + (originalRect.height() - targetHeight) / 2,
                     targetWidth, targetHeight);

    QPropertyAnimation* animation = new QPropertyAnimation(widget, "geometry");
    animation->setDuration(duration);
    animation->setStartValue(originalRect);
    animation->setEndValue(targetRect);
    animation->setEasingCurve(QEasingCurve::InOutSine);
    animation->setLoopCount(-1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    {
        std::lock_guard<std::mutex> lock(s_animationMutex);
        s_animationCache[widget]["breathing"] = animation;
    }

    QObject::connect(widget, &QObject::destroyed, [widget]() {
        cleanupWidgetAnimations(widget);
    });
}

void Animation::stopBreathing(QWidget* widget) {
    QPointer<QWidget> wp(widget);
    std::lock_guard<std::mutex> lock(s_animationMutex);
    if (s_animationCache.contains(wp) && s_animationCache[wp].contains("breathing")) {
        QPointer<QPropertyAnimation> anim = s_animationCache[wp]["breathing"];
        s_animationCache[wp].remove("breathing");
        if (anim) {
            // stop() 会触发 DeleteWhenStopped 自动删除,无需 deleteLater
            anim->stop();
        }
    }
}

void Animation::applyGlow(QWidget* widget, const QColor& color, int duration) {
    QPointer<QWidget> wp(widget);
    {
        std::lock_guard<std::mutex> lock(s_animationMutex);
        QPointer<QGraphicsDropShadowEffect> existingEffect = s_glowEffectCache.value(wp, nullptr);
        if (existingEffect) {
            s_glowEffectCache.remove(wp);
            widget->setGraphicsEffect(nullptr);
        }
    }

    auto glowEffect = std::make_unique<QGraphicsDropShadowEffect>();
    glowEffect->setColor(color);
    glowEffect->setBlurRadius(0);
    glowEffect->setOffset(0);
    widget->setGraphicsEffect(glowEffect.get());
    {
        std::lock_guard<std::mutex> lock(s_animationMutex);
        s_glowEffectCache[wp] = glowEffect.get();
    }
    QGraphicsDropShadowEffect* effectPtr = glowEffect.release();

    QPropertyAnimation* animation = new QPropertyAnimation(effectPtr, "blurRadius");
    animation->setDuration(duration);
    animation->setStartValue(0);
    animation->setEndValue(design::GLOW_MAX_BLUR_RADIUS);
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    QObject::connect(widget, &QObject::destroyed, [widget]() {
        cleanupWidgetAnimations(widget);
    });
}

void Animation::removeGlow(QWidget* widget) {
    QPointer<QWidget> wp(widget);
    QPointer<QGraphicsDropShadowEffect> effect;
    {
        std::lock_guard<std::mutex> lock(s_animationMutex);
        effect = s_glowEffectCache.value(wp, nullptr);
    }
    if (effect) {
        QPropertyAnimation* animation = new QPropertyAnimation(effect.data(), "blurRadius");
        animation->setDuration(design::ANIMATION_DURATION_SHORT);
        animation->setStartValue(effect->blurRadius());
        animation->setEndValue(0);
        animation->setEasingCurve(QEasingCurve::InOutQuad);
        QObject::connect(animation, &QPropertyAnimation::finished, [wp]() {
            if (wp) {
                wp->setGraphicsEffect(nullptr);
            }
            std::lock_guard<std::mutex> lock(s_animationMutex);
            s_glowEffectCache.remove(wp);
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        QGraphicsEffect* graphicsEffect = widget->graphicsEffect();
        if (graphicsEffect) {
            auto shadowEffect = dynamic_cast<QGraphicsDropShadowEffect*>(graphicsEffect);
            if (shadowEffect) {
                QPropertyAnimation* animation = new QPropertyAnimation(shadowEffect, "blurRadius");
                animation->setDuration(design::ANIMATION_DURATION_SHORT);
                animation->setStartValue(shadowEffect->blurRadius());
                animation->setEndValue(0);
                animation->setEasingCurve(QEasingCurve::InOutQuad);
                QObject::connect(animation, &QPropertyAnimation::finished, [wp]() {
                    if (wp) {
                        wp->setGraphicsEffect(nullptr);
                    }
                });
                animation->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
    }
}

void Animation::applyPress(QWidget* widget, int duration) {
    QRect originalRect = widget->geometry();
    int pressedWidth = originalRect.width() * (design::PRESS_SCALE_FACTOR / 100.0);
    int pressedHeight = originalRect.height() * (design::PRESS_SCALE_FACTOR / 100.0);
    
    QRect pressedRect(originalRect.left() + (originalRect.width() - pressedWidth) / 2,
                      originalRect.top() + (originalRect.height() - pressedHeight) / 2,
                      pressedWidth, pressedHeight);

    QPropertyAnimation* pressAnim = new QPropertyAnimation(widget, "geometry");
    pressAnim->setDuration(duration / 2);
    pressAnim->setStartValue(originalRect);
    pressAnim->setEndValue(pressedRect);
    pressAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* releaseAnim = new QPropertyAnimation(widget, "geometry");
    releaseAnim->setDuration(duration / 2);
    releaseAnim->setStartValue(pressedRect);
    releaseAnim->setEndValue(originalRect);
    releaseAnim->setEasingCurve(QEasingCurve::OutElastic);

    QSequentialAnimationGroup* group = new QSequentialAnimationGroup();
    group->addAnimation(pressAnim);
    group->addAnimation(releaseAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void Animation::applyHoverLift(QWidget* widget, int duration) {
    QPointer<QWidget> wp(widget);
    {
        std::lock_guard<std::mutex> lock(s_animationMutex);
        if (s_animationCache.contains(wp) && s_animationCache[wp].contains("hover_lift")) {
            QPointer<QPropertyAnimation> oldAnim = s_animationCache[wp]["hover_lift"];
            s_animationCache[wp].remove("hover_lift");
            if (oldAnim) {
                // stop() 会触发 DeleteWhenStopped 自动删除,无需 deleteLater
                oldAnim->stop();
            }
        }
    }

    if (!widget->property("_base_geometry").isValid()) {
        widget->setProperty("_base_geometry", QVariant::fromValue(widget->geometry()));
    }

    QRect baseRect = widget->property("_base_geometry").value<QRect>();

    QPropertyAnimation* animation = new QPropertyAnimation(widget, "geometry");
    animation->setDuration(duration);
    animation->setStartValue(widget->geometry());
    animation->setEndValue(QRect(baseRect.left(), baseRect.top() - design::HOVER_LIFT_OFFSET,
                                 baseRect.width(), baseRect.height()));
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    {
        std::lock_guard<std::mutex> lock(s_animationMutex);
        s_animationCache[wp]["hover_lift"] = animation;
    }

    QObject::connect(widget, &QObject::destroyed, [widget]() {
        cleanupWidgetAnimations(widget);
    });
}

void Animation::removeHoverLift(QWidget* widget) {
    QPointer<QWidget> wp(widget);
    {
        std::lock_guard<std::mutex> lock(s_animationMutex);
        if (s_animationCache.contains(wp) && s_animationCache[wp].contains("hover_lift")) {
            QPointer<QPropertyAnimation> anim = s_animationCache[wp]["hover_lift"];
            s_animationCache[wp].remove("hover_lift");
            if (anim) {
                // stop() 会触发 DeleteWhenStopped 自动删除,无需 deleteLater
                anim->stop();
            }
        }
    }

    if (widget->property("_base_geometry").isValid()) {
        QRect baseRect = widget->property("_base_geometry").value<QRect>();

        QPropertyAnimation* animation = new QPropertyAnimation(widget, "geometry");
        animation->setDuration(design::ANIMATION_DURATION_SHORT);
        animation->setStartValue(widget->geometry());
        animation->setEndValue(baseRect);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void Animation::animate(QWidget* widget, const QByteArray& propertyName,
                        const QVariant& startValue, const QVariant& endValue,
                        int duration, QEasingCurve::Type curve) {
    QPropertyAnimation* animation = new QPropertyAnimation(widget, propertyName);
    animation->setStartValue(startValue);
    animation->setEndValue(endValue);
    animation->setDuration(duration);
    animation->setEasingCurve(curve);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QPropertyAnimation* Animation::getOrCreateAnimation(QWidget* widget, const QString& key) {
    QPointer<QWidget> wp(widget);
    std::lock_guard<std::mutex> lock(s_animationMutex);
    if (!s_animationCache.contains(wp)) {
        s_animationCache[wp] = QMap<QString, QPointer<QPropertyAnimation>>();
    }

    if (s_animationCache[wp].contains(key)) {
        return s_animationCache[wp][key].data();
    }

    return nullptr;
}

}
