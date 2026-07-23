#ifndef SOUL_UI_BADGE_H
#define SOUL_UI_BADGE_H

#include "soul/ui/theme.h"
#include <QWidget>

namespace sc {

class Badge : public QWidget {
    Q_OBJECT
public:
    explicit Badge(QWidget* parent = nullptr);

    int count() const;
    void setCount(int count);

    bool isVisible() const;
    void setVisible(bool visible);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    int m_count;
    bool m_visible;
};

}

#endif