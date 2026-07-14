#include "soul/base/base_widget.h"

namespace sc {

BaseWidget::BaseWidget(QWidget* parent) : QWidget(parent) {
    connectThemeChanged();
    applyTheme();
}

void BaseWidget::applyTheme() {
    Style& style = Theme::instance().style();
    setStyleSheet(QString(
        "QWidget {"
        "   background-color: %1;"
        "   color: %2;"
        "}"
    ).arg(
        style.color(ColorRole::Surface).name(),
        style.color(ColorRole::TextPrimary).name()
    ));
}

void BaseWidget::connectThemeChanged() {
    connect(&Theme::instance(), &Theme::themeChanged, this, &BaseWidget::onThemeChanged);
}

void BaseWidget::onThemeChanged() {
    applyTheme();
}

}