#ifndef SOUL_UI_IGLASS_EFFECT_H
#define SOUL_UI_IGLASS_EFFECT_H

#include <QColor>
#include <QSize>

namespace sc {

class IGlassEffect {
public:
    virtual ~IGlassEffect() = default;

    virtual void setBlurRadius(int radius) = 0;
    virtual int blurRadius() const = 0;

    virtual void setOpacity(qreal opacity) = 0;
    virtual qreal opacity() const = 0;

    virtual void setTintColor(const QColor& color) = 0;
    virtual QColor tintColor() const = 0;

    virtual void update() = 0;
};

}

#endif