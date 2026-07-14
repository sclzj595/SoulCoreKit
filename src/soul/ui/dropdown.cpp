#include "soul/ui/dropdown.h"
#include <QPainter>
#include <QPainterPath>

namespace sc {

Dropdown::Dropdown(QWidget* parent) : QComboBox(parent) {
    setStyleSheet(R"(
        QComboBox {
            background-color: rgba(30, 41, 59, 0.8);
            color: #f8fafc;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 8px 32px 8px 12px;
            min-height: 36px;
        }
        QComboBox:hover {
            border-color: #6366f1;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox::down-arrow {
            image: none;
        }
        QComboBox QAbstractItemView {
            background-color: #1e293b;
            color: #f8fafc;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 4px;
            selection-background-color: rgba(99, 102, 241, 0.3);
        }
    )");
}

void Dropdown::paintEvent(QPaintEvent* event) {
    QComboBox::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const Style& s = Theme::instance().style();
    int arrowSize = 8;
    int arrowX = width() - arrowSize - 12;
    int arrowY = (height() - arrowSize) / 2;

    QPainterPath arrowPath;
    arrowPath.moveTo(arrowX, arrowY);
    arrowPath.lineTo(arrowX + arrowSize / 2, arrowY + arrowSize);
    arrowPath.lineTo(arrowX + arrowSize, arrowY);
    arrowPath.closeSubpath();

    painter.setBrush(s.color(ColorRole::TextSecondary));
    painter.setPen(Qt::NoPen);
    painter.drawPath(arrowPath);
}

}
