#ifndef SOUL_UI_TOOL_TIP_H
#define SOUL_UI_TOOL_TIP_H

#include "soul/ui/theme.h"
#include <QWidget>

namespace sc {

class ToolTip : public QWidget {
    Q_OBJECT
public:
    explicit ToolTip(QWidget* parent = nullptr);

    QString text() const;
    void setText(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    QString m_text;
};

}

#endif