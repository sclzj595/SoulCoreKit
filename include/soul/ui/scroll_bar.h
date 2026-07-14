#ifndef SOUL_UI_SCROLL_BAR_H
#define SOUL_UI_SCROLL_BAR_H

#include "soul/ui/theme.h"
#include <QScrollBar>

namespace sc {

class ScrollBar : public QScrollBar {
    Q_OBJECT
public:
    explicit ScrollBar(QWidget* parent = nullptr);
    explicit ScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool m_isDragging;
};

}

#endif