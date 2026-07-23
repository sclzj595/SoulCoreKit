#ifndef SOUL_UI_TAB_BAR_H
#define SOUL_UI_TAB_BAR_H

#include "soul/ui/theme.h"
#include <QTabBar>

namespace sc {

class TabBar : public QTabBar {
    Q_OBJECT
public:
    explicit TabBar(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void paintTab(QPainter* painter, int index, const QRect& rect) const;
};

}

#endif