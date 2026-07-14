#include "soul/ui/page.h"
#include <QVBoxLayout>

namespace sc {

Page::Page(QWidget* parent) : BaseWidget(parent) {}

void Page::setPageTitle(const QString& title) {
    m_title = title;
}

QString Page::pageTitle() const {
    return m_title;
}

void Page::setPageSubtitle(const QString& subtitle) {
    m_subtitle = subtitle;
}

QString Page::pageSubtitle() const {
    return m_subtitle;
}

void Page::onEnter() {
    emit pageEnter();
}

void Page::onLeave() {
    emit pageLeave();
}

void Page::onBack() {
    emit backPressed();
}

void Page::setupContent(QWidget* content) {
    QLayout* layout = new QVBoxLayout(this);
    layout->addWidget(content);
}

}