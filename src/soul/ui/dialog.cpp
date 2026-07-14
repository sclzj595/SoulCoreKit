#include "soul/ui/dialog.h"
#include "soul/ui/theme.h"
#include <QHBoxLayout>
#include <QSpacerItem>

namespace sc {

Dialog::Dialog(QWidget* parent) : QDialog(parent), m_type(DialogType::Info) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setupUI();
}

void Dialog::setType(DialogType type) {
    m_type = type;
}

void Dialog::setTitle(const QString& title) {
    m_title = title;
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void Dialog::setMessage(const QString& message) {
    m_message = message;
    if (m_messageLabel) {
        m_messageLabel->setText(message);
    }
}

void Dialog::addButton(DialogButton button, const QString& text) {
    if (!m_buttonLayout) {
        return;
    }

    QString btnText = text;
    if (btnText.isEmpty()) {
        switch (button) {
            case DialogButton::Ok: btnText = "OK"; break;
            case DialogButton::Cancel: btnText = "Cancel"; break;
            case DialogButton::Yes: btnText = "Yes"; break;
            case DialogButton::No: btnText = "No"; break;
        }
    }

    QPushButton* btn = new QPushButton(btnText);
    btn->setFixedHeight(36);
    btn->setMinimumWidth(80);

    Style& style = Theme::instance().style();
    btn->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: none;"
        "   border-radius: %3px;"
        "   font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "   background-color: %4;"
        "}"
        "QPushButton:pressed {"
        "   background-color: %5;"
        "}"
    ).arg(
        style.color(ColorRole::Primary).name(),
        style.color(ColorRole::OnPrimary).name(),
        QString::number(style.cornerRadius(CornerRadius::Small)),
        style.color(ColorRole::PrimaryLight).name(),
        style.color(ColorRole::PrimaryDark).name()
    ));

    connect(btn, &QPushButton::clicked, this, [this, button]() {
        if (m_callback) {
            m_callback(button);
        }
        done(static_cast<int>(button));
    });

    m_buttonLayout->addWidget(btn);
}

void Dialog::setOnButtonClicked(std::function<void(DialogButton)> callback) {
    m_callback = callback;
}

void Dialog::setupUI() {
    Style& style = Theme::instance().style();

    setStyleSheet(QString(
        "Dialog {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "}"
    ).arg(
        style.color(ColorRole::Surface).name(),
        QString::number(style.cornerRadius(CornerRadius::Large))
    ));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(24, 24, 24, 24);
    m_mainLayout->setSpacing(16);

    QHBoxLayout* iconLayout = new QHBoxLayout();
    iconLayout->setAlignment(Qt::AlignCenter);

    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(48, 48);
    iconLayout->addWidget(m_iconLabel);

    m_mainLayout->addLayout(iconLayout);

    m_titleLabel = new QLabel(m_title);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(QString(
        "QLabel {"
        "   color: %1;"
        "   font-size: 16px;"
        "   font-weight: 600;"
        "}"
    ).arg(style.color(ColorRole::TextPrimary).name()));
    m_mainLayout->addWidget(m_titleLabel);

    m_messageLabel = new QLabel(m_message);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet(QString(
        "QLabel {"
        "   color: %1;"
        "   font-size: 14px;"
        "}"
    ).arg(style.color(ColorRole::TextSecondary).name()));
    m_mainLayout->addWidget(m_messageLabel);

    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(12);
    m_buttonLayout->setAlignment(Qt::AlignCenter);

    m_mainLayout->addLayout(m_buttonLayout);

    setFixedSize(360, 240);
}

int Dialog::info(QWidget* parent, const QString& title, const QString& message) {
    Dialog dialog(parent);
    dialog.setType(DialogType::Info);
    dialog.setTitle(title);
    dialog.setMessage(message);
    dialog.addButton(DialogButton::Ok);
    return dialog.exec();
}

int Dialog::warning(QWidget* parent, const QString& title, const QString& message) {
    Dialog dialog(parent);
    dialog.setType(DialogType::Warning);
    dialog.setTitle(title);
    dialog.setMessage(message);
    dialog.addButton(DialogButton::Ok);
    return dialog.exec();
}

int Dialog::error(QWidget* parent, const QString& title, const QString& message) {
    Dialog dialog(parent);
    dialog.setType(DialogType::Error);
    dialog.setTitle(title);
    dialog.setMessage(message);
    dialog.addButton(DialogButton::Ok);
    return dialog.exec();
}

int Dialog::success(QWidget* parent, const QString& title, const QString& message) {
    Dialog dialog(parent);
    dialog.setType(DialogType::Success);
    dialog.setTitle(title);
    dialog.setMessage(message);
    dialog.addButton(DialogButton::Ok);
    return dialog.exec();
}

int Dialog::confirm(QWidget* parent, const QString& title, const QString& message) {
    Dialog dialog(parent);
    dialog.setType(DialogType::Confirm);
    dialog.setTitle(title);
    dialog.setMessage(message);
    dialog.addButton(DialogButton::Yes);
    dialog.addButton(DialogButton::No);
    return dialog.exec();
}

QString Dialog::getIconPath(DialogType type) const {
    switch (type) {
        case DialogType::Info: return ":/icons/info.png";
        case DialogType::Warning: return ":/icons/warning.png";
        case DialogType::Error: return ":/icons/error.png";
        case DialogType::Success: return ":/icons/success.png";
        case DialogType::Confirm: return ":/icons/confirm.png";
        default: return ":/icons/info.png";
    }
}

}