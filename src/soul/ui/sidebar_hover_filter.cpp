#include "soul/ui/sidebar_hover_filter.h"
#include "soul/ui/animation.h"
#include "soul/ui/theme.h"
#include <QPushButton>

namespace sc {

bool SideBarHoverFilter::eventFilter(QObject* obj, QEvent* event) {
    QPushButton* button = qobject_cast<QPushButton*>(obj);
    if (!button) return QObject::eventFilter(obj, event);

    if (event->type() == QEvent::Enter) {
        Animation::applyGlow(button, Theme::instance().style().color(ColorRole::Primary), 150);
        Animation::applyBreathing(button, 1000);
    } else if (event->type() == QEvent::Leave) {
        Animation::removeGlow(button);
        Animation::stopBreathing(button);
    }

    return QObject::eventFilter(obj, event);
}

}
