#include "soul/ui/loading.h"
#include <QApplication>

namespace sc {

// [审计] showGlobal/hideGlobal/updateGlobalProgress 原本各自定义独立的
// static Loading*, hideGlobal 永远找不到 showGlobal 创建的对象→无法关闭。
// 提取为文件级静态, 三函数共享同一指针。
namespace {
    Loading* g_globalLoading = nullptr;
}

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
    if (!g_globalLoading) {
        g_globalLoading = new Loading(qApp->activeWindow());
    }
    g_globalLoading->setText(text);
    g_globalLoading->show();
}

void Loading::hideGlobal() {
    if (g_globalLoading) {
        g_globalLoading->hide();
    }
}

void Loading::updateGlobalProgress(int value) {
    if (g_globalLoading) {
        g_globalLoading->setProgress(value);
    }
}

}
