#ifndef SOUL_UI_WINDOW_H
#define SOUL_UI_WINDOW_H

#include <QMainWindow>
#include <QString>
#include <QColor>

namespace sc {

class Window : public QMainWindow {
    Q_OBJECT
    Q_PROPERTY(bool frameless READ isFrameless WRITE setFrameless)
    Q_PROPERTY(bool glassEffect READ hasGlassEffect WRITE setGlassEffect)
    Q_PROPERTY(bool glowBorder READ hasGlowBorder WRITE setGlowBorder)

public:
    explicit Window(QWidget* parent = nullptr);
    ~Window() override = default;

    void setTitle(const QString& title);
    QString title() const;

    void setWindowIcon(const QIcon& icon);

    void showCentered();
    void showFullScreen();
    void showMaximized();
    void showMinimized();

    void setFixedSize(int width, int height);
    void setMinimumSize(int width, int height);
    void setMaximumSize(int width, int height);

    void setFrameless(bool enabled);
    bool isFrameless() const;

    void setGlassEffect(bool enabled);
    bool hasGlassEffect() const;

    void setGlowBorder(bool enabled);
    bool hasGlowBorder() const;

    void setBlurRadius(int radius);
    int blurRadius() const;

    void setTintColor(const QColor& color);
    QColor tintColor() const;

signals:
    void windowClosed();
    void windowMinimized();
    void windowMaximized();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void updateStyle();

    QString m_title;
    bool m_frameless;
    bool m_glassEffect;
    bool m_glowBorder;
    int m_blurRadius;
    QColor m_tintColor;
    QPoint m_dragPosition;
};

}

#endif
