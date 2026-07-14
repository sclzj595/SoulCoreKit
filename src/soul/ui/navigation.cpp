#include "soul/ui/navigation.h"

namespace sc {

QStackedWidget* Navigation::s_stack = nullptr;
QStringList Navigation::s_titles;
Navigation::NavigationCallback Navigation::s_onNavigate = nullptr;

void Navigation::setRootWidget(QStackedWidget* stack) {
    s_stack = stack;
}

void Navigation::push(QWidget* widget, const QString& title) {
    if (!s_stack) {
        return;
    }
    s_stack->addWidget(widget);
    s_titles.append(title);
    s_stack->setCurrentWidget(widget);
    if (s_onNavigate) {
        s_onNavigate(widget, title);
    }
}

void Navigation::pop() {
    if (!s_stack || s_stack->count() <= 1) {
        return;
    }
    QWidget* current = s_stack->currentWidget();
    s_stack->removeWidget(current);
    s_titles.removeLast();
    s_stack->setCurrentIndex(s_stack->count() - 1);
    if (s_onNavigate) {
        s_onNavigate(s_stack->currentWidget(), s_titles.isEmpty() ? "" : s_titles.last());
    }
}

void Navigation::popToRoot() {
    if (!s_stack) {
        return;
    }
    while (s_stack->count() > 1) {
        QWidget* widget = s_stack->widget(s_stack->count() - 1);
        s_stack->removeWidget(widget);
        s_titles.removeLast();
    }
    s_stack->setCurrentIndex(0);
    if (s_onNavigate) {
        s_onNavigate(s_stack->currentWidget(), s_titles.isEmpty() ? "" : s_titles.first());
    }
}

void Navigation::replace(QWidget* widget, const QString& title) {
    if (!s_stack || s_stack->count() == 0) {
        return;
    }
    QWidget* current = s_stack->currentWidget();
    s_stack->removeWidget(current);
    s_stack->insertWidget(s_stack->currentIndex(), widget);
    s_titles[s_stack->currentIndex()] = title;
    s_stack->setCurrentWidget(widget);
    if (s_onNavigate) {
        s_onNavigate(widget, title);
    }
}

int Navigation::currentIndex() {
    if (!s_stack) {
        return -1;
    }
    return s_stack->currentIndex();
}

QWidget* Navigation::currentWidget() {
    if (!s_stack) {
        return nullptr;
    }
    return s_stack->currentWidget();
}

QString Navigation::currentTitle() {
    if (s_titles.isEmpty()) {
        return "";
    }
    return s_titles.last();
}

int Navigation::stackCount() {
    if (!s_stack) {
        return 0;
    }
    return s_stack->count();
}

void Navigation::setOnNavigate(NavigationCallback callback) {
    s_onNavigate = callback;
}

}