#include "soul/ui/avatar.h"
#include <QPainter>
#include <QPainterPath>

namespace sc {

Avatar::Avatar(QWidget* parent) : QWidget(parent), m_size(40) {
    setFixedSize(m_size, m_size);
}

QPixmap Avatar::pixmap() const {
    return m_pixmap;
}

void Avatar::setPixmap(const QPixmap& pixmap) {
    m_pixmap = pixmap;
    update();
}

QString Avatar::initials() const {
    return m_initials;
}

void Avatar::setInitials(const QString& initials) {
    m_initials = initials;
    m_pixmap = QPixmap();
    update();
}

int Avatar::size() const {
    return m_size;
}

void Avatar::setSize(int size) {
    m_size = size;
    setFixedSize(size, size);
    update();
}

void Avatar::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const Style& s = Theme::instance().style();
    int radius = m_size / 2;

    if (!m_pixmap.isNull()) {
        QPixmap scaled = m_pixmap.scaled(m_size, m_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPainterPath path;
        path.addEllipse(rect());
        painter.setClipPath(path);
        painter.drawPixmap(rect(), scaled);
    } else {
        QLinearGradient gradient(rect().topLeft(), rect().bottomRight());
        gradient.setColorAt(0, s.color(ColorRole::Primary));
        gradient.setColorAt(1, s.color(ColorRole::PrimaryDark));
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect());

        painter.setPen(s.color(ColorRole::OnPrimary));
        QFont font = s.font();
        font.setPointSize(m_size / 3);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, m_initials.isEmpty() ? "?" : m_initials);
    }

    QColor borderColor = s.color(ColorRole::Surface);
    painter.setPen(QPen(borderColor, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(rect().adjusted(1, 1, -1, -1));
}

QSize Avatar::sizeHint() const {
    return QSize(m_size, m_size);
}

}
