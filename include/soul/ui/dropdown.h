#ifndef SOUL_UI_DROPDOWN_H
#define SOUL_UI_DROPDOWN_H

#include "soul/ui/theme.h"
#include <QComboBox>

namespace sc {

class Dropdown : public QComboBox {
    Q_OBJECT
public:
    explicit Dropdown(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

}

#endif