#ifndef SOUL_UI_AVATAR_H
#define SOUL_UI_AVATAR_H

#include "soul/ui/theme.h"
#include <QWidget>

namespace sc {

class Avatar : public QWidget {
    Q_OBJECT
public:
    explicit Avatar(QWidget* parent = nullptr);

    QPixmap pixmap() const;
    void setPixmap(const QPixmap& pixmap);

    QString initials() const;
    void setInitials(const QString& initials);

    int size() const;
    void setSize(int size);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    QPixmap m_pixmap;
    QString m_initials;
    int m_size;
};

}

#endif