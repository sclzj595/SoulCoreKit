#include "soul/ui/loading.h"
#include <QApplication>

namespace sc {

Loading::Loading(QWidget* parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setAlignment(Qt::AlignCenter);

    m_textLabel = new QLabel("Loading...", this);
    m_layout->addWidget(m_textLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->hide();
    m_layout->addWidget(m_progressBar);
}

void Loading::setText(const QString& text) {
    m_textLabel->setText(text);
}

QString Loading::text() const {
    return m_textLabel->text();
}

void Loading::showProgress(bool show) {
    m_progressBar->setVisible(show);
}

void Loading::setProgress(int value) {
    m_progressBar->setValue(value);
}

int Loading::progress() const {
    return m_progressBar->value();
}

void Loading::setIndeterminate(bool indeterminate) {
    m_progressBar->setRange(indeterminate ? 0 : 0, indeterminate ? 0 : 100);
}

void Loading::showGlobal(const QString& text) {
    static Loading* globalLoading = nullptr;
    if (!globalLoading) {
        globalLoading = new Loading(qApp->activeWindow());
    }
    globalLoading->setText(text);
    globalLoading->show();
}

void Loading::hideGlobal() {
    static Loading* globalLoading = nullptr;
    if (globalLoading) {
        globalLoading->hide();
    }
}

void Loading::updateGlobalProgress(int value) {
    static Loading* globalLoading = nullptr;
    if (globalLoading) {
        globalLoading->setProgress(value);
    }
}

}