#include "soul/ui/empty_widget.h"

namespace sc {

EmptyWidget::EmptyWidget(QWidget* parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setAlignment(Qt::AlignCenter);
    m_layout->setSpacing(12);

    m_iconLabel = new QLabel(this);
    m_layout->addWidget(m_iconLabel);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_subtitleLabel);

    m_actionButton = new QPushButton(this);
    connect(m_actionButton, &QPushButton::clicked, this, &EmptyWidget::buttonClicked);
    m_actionButton->hide();
    m_layout->addWidget(m_actionButton);
}

void EmptyWidget::setIcon(const QIcon& icon) {
    m_iconLabel->setPixmap(icon.pixmap(64, 64));
}

void EmptyWidget::setTitle(const QString& title) {
    m_titleLabel->setText(title);
}

void EmptyWidget::setSubtitle(const QString& subtitle) {
    m_subtitleLabel->setText(subtitle);
}

void EmptyWidget::setButtonText(const QString& text) {
    m_actionButton->setText(text);
}

void EmptyWidget::showButton(bool show) {
    m_actionButton->setVisible(show);
}

}