#ifndef SOUL_UI_GLASS_WIDGET_H
#define SOUL_UI_GLASS_WIDGET_H

#include <QWidget>
#include <QColor>
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QPixmap>
#include "i_glass_effect.h"

namespace sc {
namespace ui {

class GlassWidget : public QWidget, public IGlassEffect {
    Q_OBJECT
    Q_PROPERTY(int blurRadius READ blurRadius WRITE setBlurRadius)
    Q_PROPERTY(qreal glassOpacity READ opacity WRITE setOpacity)
    Q_PROPERTY(QColor tintColor READ tintColor WRITE setTintColor)

public:
    explicit GlassWidget(QWidget* parent = nullptr);
    ~GlassWidget() override = default;

    void setBlurRadius(int radius) override;
    int blurRadius() const override;

    void setOpacity(qreal opacity) override;
    qreal opacity() const override;

    void setTintColor(const QColor& color) override;
    QColor tintColor() const override;

    void update() override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* event) override;

private:
    void updateBackground();

    int m_blurRadius;
    qreal m_opacity;
    QColor m_tintColor;
    QPixmap m_blurredBackground;
};

} // namespace ui
} // namespace sc

#endif
