#ifndef SOUL_UI_CARD_H
#define SOUL_UI_CARD_H

#include <QWidget>
#include <QVBoxLayout>
#include <QString>
#include <QColor>

namespace sc {

class Card : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int borderRadius READ borderRadius WRITE setBorderRadius)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(bool hoverEnabled READ isHoverEnabled WRITE setHoverEnabled)

public:
    explicit Card(QWidget* parent = nullptr);
    ~Card() override = default;

    void setBorderRadius(int radius);
    int borderRadius() const;

    void setOpacity(qreal opacity);
    qreal opacity() const;

    void setHoverEnabled(bool enabled);
    bool isHoverEnabled() const;

    void setTintColor(const QColor& color);
    QColor tintColor() const;

    void setBlurRadius(int radius);
    int blurRadius() const;

    QLayout* contentLayout();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateStyle();

    int m_borderRadius;
    qreal m_opacity;
    bool m_hoverEnabled;
    QColor m_tintColor;
    int m_blurRadius;
    QVBoxLayout* m_layout;
};

}

#endif