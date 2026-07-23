#ifndef SOUL_UI_SIDEBAR_HOVER_FILTER_H
#define SOUL_UI_SIDEBAR_HOVER_FILTER_H

#include <QObject>
#include <QEvent>

namespace sc {

class SideBarHoverFilter : public QObject {
    Q_OBJECT
public:
    explicit SideBarHoverFilter(QObject* parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};

}

#endif