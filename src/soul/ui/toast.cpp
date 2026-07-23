#include "soul/ui/toast.h"
#include "soul/ui/theme.h"
#include "soul/ui/design_constants.h"
#include <QApplication>
#include <QScreen>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QCloseEvent>

namespace sc {

Toast::Toast(QWidget* parent) : QWidget(parent), m_type(ToastType::Info), m_duration(3000) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setupUI();
}

Toast::~Toast() {
}

void Toast::closeEvent(QCloseEvent* event) {
    QWidget::closeEvent(event);
    deleteLater();
}

void Toast::setType(ToastType type) {
    m_type = type;
}

void Toast::setMessage(const QString& message) {
    m_message = message;
    QLabel* label = findChild<QLabel*>("messageLabel");
    if (label) {
        label->setText(message);
    }
}

void Toast::setDuration(int milliseconds) {
    m_duration = milliseconds;
}

void Toast::show() {
    ToastManager::instance().addToast(std::unique_ptr<Toast>(this));
    QWidget::show();

    if (m_duration > 0) {
        m_timer = std::make_unique<QTimer>(this);
        m_timer->setSingleShot(true);
        m_timer->setInterval(m_duration);
        connect(m_timer.get(), &QTimer::timeout, this, &QWidget::close);
        m_timer->start();
    }
}

void Toast::setupUI() {
    Style& style = Theme::instance().style();

    QColor bgColor;
    switch (m_type) {
        case ToastType::Info: bgColor = style.color(ColorRole::Primary); break;
        case ToastType::Warning: bgColor = style.color(ColorRole::Secondary); break;
        case ToastType::Error: bgColor = style.color(ColorRole::Error); break;
        case ToastType::Success: bgColor = style.color(ColorRole::Success); break;
        default: bgColor = style.color(ColorRole::Primary); break;
    }

    setStyleSheet(QString(
        "Toast {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "   padding: %3px %4px;"
        "}"
    ).arg(
        bgColor.name(),
        QString::number(style.cornerRadius(CornerRadius::Medium)),
        QString::number(design::TOAST_PADDING_VERTICAL),
        QString::number(design::TOAST_PADDING_HORIZONTAL)
    ));

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(design::CARD_CONTENT_SPACING);

    QLabel* iconLabel = new QLabel();
    iconLabel->setFixedSize(design::TOAST_ICON_SIZE, design::TOAST_ICON_SIZE);
    layout->addWidget(iconLabel);

    QLabel* messageLabel = new QLabel(m_message);
    messageLabel->setObjectName("messageLabel");
    messageLabel->setStyleSheet(QString(
        "QLabel {"
        "   color: white;"
        "   font-size: %1px;"
        "}"
    ).arg(QString::number(design::TOAST_FONT_SIZE)));
    layout->addWidget(messageLabel);

    adjustSize();
}

void Toast::info(const QString& message, int duration) {
    auto toast = std::make_unique<Toast>();
    toast->setType(ToastType::Info);
    toast->setMessage(message);
    toast->setDuration(duration);
    toast->show();
}

void Toast::warning(const QString& message, int duration) {
    auto toast = std::make_unique<Toast>();
    toast->setType(ToastType::Warning);
    toast->setMessage(message);
    toast->setDuration(duration);
    toast->show();
}

void Toast::error(const QString& message, int duration) {
    auto toast = std::make_unique<Toast>();
    toast->setType(ToastType::Error);
    toast->setMessage(message);
    toast->setDuration(duration);
    toast->show();
}

void Toast::success(const QString& message, int duration) {
    auto toast = std::make_unique<Toast>();
    toast->setType(ToastType::Success);
    toast->setMessage(message);
    toast->setDuration(duration);
    toast->show();
}

ToastManager::ToastManager() : QWidget(nullptr) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);

    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->geometry();
    setGeometry(screenRect.width() - 320, screenRect.height() - 100, 300, 60);
}

ToastManager::~ToastManager() {
}

ToastManager& ToastManager::instance() {
    static ToastManager instance;
    return instance;
}

void ToastManager::addToast(std::unique_ptr<Toast> toast) {
    Toast* raw = toast.release();
    m_layout->addWidget(raw);
    show();
}

}
