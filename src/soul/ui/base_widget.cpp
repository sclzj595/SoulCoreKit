#include "soul/ui/base_widget.h"
#include "soul/ui/theme.h"

namespace sc {
namespace ui {

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

} // namespace ui
} // namespace sc
