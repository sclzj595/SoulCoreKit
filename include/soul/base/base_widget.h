#ifndef SOUL_BASE_BASE_WIDGET_H
#define SOUL_BASE_BASE_WIDGET_H

#include <QWidget>
#include "soul/ui/theme.h"

namespace sc {

class BaseWidget : public QWidget {
    Q_OBJECT
public:
    explicit BaseWidget(QWidget* parent = nullptr);
    ~BaseWidget() override = default;

protected:
    void applyTheme();
    void connectThemeChanged();

private slots:
    void onThemeChanged();
};

}

#endif