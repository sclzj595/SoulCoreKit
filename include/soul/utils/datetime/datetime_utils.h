#ifndef SOUL_UTILS_DATETIME_DATETIME_UTILS_H
#define SOUL_UTILS_DATETIME_DATETIME_UTILS_H

#include <QDateTime>
#include <QString>

namespace sc {

class DateTimeUtils {
public:
    static QString format(const QDateTime& dateTime, const QString& format = "yyyy-MM-dd HH:mm:ss");
    static QString formatDate(const QDate& date, const QString& format = "yyyy-MM-dd");
    static QString formatTime(const QTime& time, const QString& format = "HH:mm:ss");

    static QString toISO8601(const QDateTime& dateTime);
    static QDateTime fromISO8601(const QString& isoString);

    static QString toRFC3339(const QDateTime& dateTime);
    static QDateTime fromRFC3339(const QString& rfcString);

    static QString toHumanReadable(const QDateTime& dateTime);
    static QString toRelativeTime(const QDateTime& dateTime);

    static qint64 toTimestamp(const QDateTime& dateTime);
    static QDateTime fromTimestamp(qint64 timestamp);

    static QDateTime now();
    static QDate today();
    static QTime currentTime();

    static bool isLeapYear(int year);
    static int daysInMonth(int year, int month);

    static QDateTime addDays(const QDateTime& dateTime, int days);
    static QDateTime addHours(const QDateTime& dateTime, int hours);
    static QDateTime addMinutes(const QDateTime& dateTime, int minutes);

    static qint64 diffDays(const QDateTime& start, const QDateTime& end);
    static qint64 diffHours(const QDateTime& start, const QDateTime& end);
    static qint64 diffMinutes(const QDateTime& start, const QDateTime& end);
};

}

#endif