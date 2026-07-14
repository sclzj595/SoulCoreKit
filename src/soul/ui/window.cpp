#include "soul/ui/window.h"
#include "soul/ui/theme.h"
#include "soul/ui/design_constants.h"
#include "soul/ui/glass_effect_cache.h"
#include <QCloseEvent>
#include <QGuiApplication>
#include <QPainter>
#include <QMouseEvent>

namespace sc {

Window::Window(QWidget* parent)
    : QMainWindow(parent),
      m_frameless(true),
      m_glassEffect(true),
      m_glowBorder(true),
      m_blurRadius(design::WINDOW_BLUR_RADIUS) {
    m_tintColor = Theme::instance().style().color(ColorRole::GlassTint);
    if (m_frameless) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    }
    setAttribute(Qt::WA_TranslucentBackground);
    updateStyle();
}

void Window::setTitle(const QString& title) {
    m_title = title;
    setWindowTitle(title);
}

QString Window::title() const {
    return m_title;
}

void Window::setWindowIcon(const QIcon& icon) {
    QMainWindow::setWindowIcon(icon);
}

void Window::showCentered() {
    show();
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
}

void Window::showFullScreen() {
    QMainWindow::showFullScreen();
}

void Window::showMaximized() {
    QMainWindow::showMaximized();
}

void Window::showMinimized() {
    QMainWindow::showMinimized();
}

void Window::setFixedSize(int width, int height) {
    QMainWindow::setFixedSize(width, height);
}

void Window::setMinimumSize(int width, int height) {
    QMainWindow::setMinimumSize(width, height);
}

void Window::setMaximumSize(int width, int height) {
    QMainWindow::setMaximumSize(width, height);
}

void Window::setFrameless(bool enabled) {
    m_frameless = enabled;
    if (enabled) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    } else {
        setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    }
    show();
}

bool Window::isFrameless() const {
    return m_frameless;
}

void Window::setGlassEffect(bool enabled) {
    m_glassEffect = enabled;
    update();
}

bool Window::hasGlassEffect() const {
    return m_glassEffect;
}

void Window::setGlowBorder(bool enabled) {
    m_glowBorder = enabled;
    update();
}

bool Window::hasGlowBorder() const {
    return m_glowBorder;
}

void Window::setBlurRadius(int radius) {
    m_blurRadius = radius;
    update();
}

int Window::blurRadius() const {
    return m_blurRadius;
}

void Window::setTintColor(const QColor& color) {
    m_tintColor = color;
    update();
}

QColor Window::tintColor() const {
    return m_tintColor;
}

void Window::closeEvent(QCloseEvent* event) {
    emit windowClosed();
    QMainWindow::closeEvent(event);
}

void Window::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (windowState() & Qt::WindowMinimized) {
            emit windowMinimized();
        } else if (windowState() & Qt::WindowMaximized) {
            emit windowMaximized();
        }
    }
    QMainWindow::changeEvent(event);
}

void Window::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_glassEffect) {
        QPixmap source = grab(QRect(0, 0, width(), height()));
        QPixmap blurredBackground = GlassEffectCache::instance().blur(source, m_blurRadius);
        painter.drawPixmap(rect(), blurredBackground);
    }

    painter.fillRect(rect(), m_tintColor);

    if (m_glowBorder && isActiveWindow()) {
        const Style& s = Theme::instance().style();
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, s.color(ColorRole::Primary).lighter(180));
        gradient.setColorAt(0.5, s.color(ColorRole::Primary).lighter(130));
        gradient.setColorAt(1, s.color(ColorRole::Primary).lighter(180));

        painter.setPen(QPen(gradient, 2));
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}

void Window::mousePressEvent(QMouseEvent* event) {
    if (m_frameless && event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
    QMainWindow::mousePressEvent(event);
}

void Window::mouseMoveEvent(QMouseEvent* event) {
    if (m_frameless && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
    QMainWindow::mouseMoveEvent(event);
}

void Window::updateStyle() {
}

}