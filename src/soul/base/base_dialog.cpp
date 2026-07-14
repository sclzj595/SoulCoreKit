#include "soul/base/base_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>

namespace sc {

BaseDialog::BaseDialog(QWidget* parent) : QDialog(parent) {
    connect(&Theme::instance(), &Theme::themeChanged, this, &BaseDialog::onThemeChanged);
    setupLayout();
    applyTheme();
}

void BaseDialog::setDialogTitle(const QString& title) {
    QLabel* titleLabel = findChild<QLabel*>("titleLabel");
    if (titleLabel) {
        titleLabel->setText(title);
    }
}

void BaseDialog::addButton(const QString& text, std::function<void()> callback) {
    if (!m_buttonLayout) {
        return;
    }

    QPushButton* btn = new QPushButton(text);
    connect(btn, &QPushButton::clicked, this, [this, callback]() {
        if (callback) {
            callback();
        }
        accept();
    });

    Style& style = Theme::instance().style();
    btn->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: none;"
        "   border-radius: %3px;"
        "   padding: 8px 16px;"
        "}"
    ).arg(
        style.color(ColorRole::Primary).name(),
        style.color(ColorRole::OnPrimary).name(),
        QString::number(style.cornerRadius(CornerRadius::Small))
    ));

    m_buttonLayout->addWidget(btn);
}

void BaseDialog::applyTheme() {
    Style& style = Theme::instance().style();
    setStyleSheet(QString(
        "BaseDialog {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "}"
    ).arg(
        style.color(ColorRole::Surface).name(),
        QString::number(style.cornerRadius(CornerRadius::Large))
    ));

    QLabel* titleLabel = findChild<QLabel*>("titleLabel");
    if (titleLabel) {
        titleLabel->setStyleSheet(QString(
            "QLabel {"
            "   color: %1;"
            "   font-size: 16px;"
            "   font-weight: 600;"
            "}"
        ).arg(style.color(ColorRole::TextPrimary).name()));
    }
}

void BaseDialog::setupLayout() {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(24, 24, 24, 24);
    m_mainLayout->setSpacing(16);

    QLabel* titleLabel = new QLabel();
    titleLabel->setObjectName("titleLabel");
    m_mainLayout->addWidget(titleLabel);

    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(12);
    m_buttonLayout->setAlignment(Qt::AlignRight);
    m_mainLayout->addLayout(m_buttonLayout);

    setFixedSize(360, 200);
}

void BaseDialog::onThemeChanged() {
    applyTheme();
}

}