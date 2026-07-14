#include "soul/utils/datetime/datetime_utils.h"

namespace sc {

QString DateTimeUtils::format(const QDateTime& dateTime, const QString& format) {
    return dateTime.toString(format);
}

QString DateTimeUtils::formatDate(const QDate& date, const QString& format) {
    return date.toString(format);
}

QString DateTimeUtils::formatTime(const QTime& time, const QString& format) {
    return time.toString(format);
}

QString DateTimeUtils::toISO8601(const QDateTime& dateTime) {
    return dateTime.toString(Qt::ISODate);
}

QDateTime DateTimeUtils::fromISO8601(const QString& isoString) {
    return QDateTime::fromString(isoString, Qt::ISODate);
}

QString DateTimeUtils::toRFC3339(const QDateTime& dateTime) {
    return dateTime.toString("yyyy-MM-dd'T'HH:mm:ssZ");
}

QDateTime DateTimeUtils::fromRFC3339(const QString& rfcString) {
    return QDateTime::fromString(rfcString, "yyyy-MM-dd'T'HH:mm:ssZ");
}

QString DateTimeUtils::toHumanReadable(const QDateTime& dateTime) {
    QDateTime now = QDateTime::currentDateTime();
    qint64 seconds = dateTime.secsTo(now);

    if (seconds < 60) {
        return "刚刚";
    } else if (seconds < 3600) {
        return QString("%1分钟前").arg(seconds / 60);
    } else if (seconds < 86400) {
        return QString("%1小时前").arg(seconds / 3600);
    } else if (seconds < 604800) {
        return QString("%1天前").arg(seconds / 86400);
    }

    return dateTime.toString("yyyy-MM-dd");
}

QString DateTimeUtils::toRelativeTime(const QDateTime& dateTime) {
    return toHumanReadable(dateTime);
}

qint64 DateTimeUtils::toTimestamp(const QDateTime& dateTime) {
    return dateTime.toSecsSinceEpoch();
}

QDateTime DateTimeUtils::fromTimestamp(qint64 timestamp) {
    return QDateTime::fromSecsSinceEpoch(timestamp);
}

QDateTime DateTimeUtils::now() {
    return QDateTime::currentDateTime();
}

QDate DateTimeUtils::today() {
    return QDate::currentDate();
}

QTime DateTimeUtils::currentTime() {
    return QTime::currentTime();
}

bool DateTimeUtils::isLeapYear(int year) {
    return QDate::isLeapYear(year);
}

int DateTimeUtils::daysInMonth(int year, int month) {
    return QDate(year, month, 1).daysInMonth();
}

QDateTime DateTimeUtils::addDays(const QDateTime& dateTime, int days) {
    return dateTime.addDays(days);
}

QDateTime DateTimeUtils::addHours(const QDateTime& dateTime, int hours) {
    return dateTime.addSecs(hours * 3600);
}

QDateTime DateTimeUtils::addMinutes(const QDateTime& dateTime, int minutes) {
    return dateTime.addSecs(minutes * 60);
}

qint64 DateTimeUtils::diffDays(const QDateTime& start, const QDateTime& end) {
    return start.daysTo(end);
}

qint64 DateTimeUtils::diffHours(const QDateTime& start, const QDateTime& end) {
    return start.secsTo(end) / 3600;
}

qint64 DateTimeUtils::diffMinutes(const QDateTime& start, const QDateTime& end) {
    return start.secsTo(end) / 60;
}

}