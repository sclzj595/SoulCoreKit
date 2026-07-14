#include "soul/ui/base_view.h"

namespace sc {

BaseView::BaseView(QWidget* parent) : QWidget(parent) {
    initUI();
    setupLayout();
}

QWidget* BaseView::widget() {
    return this;
}

void BaseView::show() {
    QWidget::show();
}

void BaseView::hide() {
    QWidget::hide();
}

void BaseView::close() {
    QWidget::close();
}

void BaseView::setTitle(const QString& title) {
    setWindowTitle(title);
}

QString BaseView::title() const {
    return windowTitle();
}

void BaseView::setSize(int width, int height) {
    resize(width, height);
}

QSize BaseView::size() const {
    return QWidget::size();
}

void BaseView::setPosition(int x, int y) {
    move(x, y);
}

QPoint BaseView::position() const {
    return QWidget::pos();
}

void BaseView::initUI() {
}

void BaseView::setupLayout() {
}

}