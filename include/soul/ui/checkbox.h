#ifndef SOUL_UI_CHECKBOX_H
#define SOUL_UI_CHECKBOX_H

#include "soul/ui/theme.h"
#include <QAbstractButton>
#include <QPropertyAnimation>

namespace sc {

class Checkbox : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
    Q_PROPERTY(qreal checkProgress READ checkProgress WRITE setCheckProgress)

public:
    explicit Checkbox(QWidget* parent = nullptr);
    explicit Checkbox(const QString& text, QWidget* parent = nullptr);

    bool isChecked() const;
    void setChecked(bool checked);

    qreal checkProgress() const;
    void setCheckProgress(qreal progress);

Q_SIGNALS:
    void toggled(bool checked);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    bool m_checked;
    qreal m_checkProgress;
    QPropertyAnimation* m_animation;
};

}

#endif