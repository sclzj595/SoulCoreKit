#include "soul/utils/string/string_utils.h"
#include <QStringList>
#include <cstdarg>
#include <cstdio>

namespace sc {
namespace utils {
namespace string {

QString trim(const QString& str) {
    return str.trimmed();
}

QString trimLeft(const QString& str) {
    return str.trimmed();
}

QString trimRight(const QString& str) {
    return str.trimmed();
}


QString toLower(const QString& str) {
    return str.toLower();
}

QString toUpper(const QString& str) {
    return str.toUpper();
}

bool startsWith(const QString& str, const QString& prefix) {
    return str.startsWith(prefix);
}

bool endsWith(const QString& str, const QString& suffix) {
    return str.endsWith(suffix);
}

bool contains(const QString& str, const QString& substring) {
    return str.contains(substring);
}

std::vector<QString> split(const QString& str, const QString& delimiter) {
    QStringList list = str.split(delimiter);
    std::vector<QString> result;
    for (const QString& item : list) {
        result.push_back(item);
    }
    return result;
}

QString join(const std::vector<QString>& parts, const QString& delimiter) {
    QStringList list;
    for (const QString& part : parts) {
        list.append(part);
    }
    return list.join(delimiter);
}

QString replace(const QString& str, const QString& oldValue, const QString& newValue) {
    QString temp = str;
    return temp.replace(oldValue, newValue);
}

QString format(const QString& format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[1024];
    int len = std::vsnprintf(buffer, sizeof(buffer), format.toUtf8().constData(), args);
    va_end(args);
    return QString::fromUtf8(buffer, len);
}

bool isEmpty(const QString& str) {
    return str.isEmpty();
}

bool isBlank(const QString& str) {
    return str.trimmed().isEmpty();
}

int count(const QString& str, const QString& substring) {
    return str.count(substring);
}

QString substring(const QString& str, int start, int length) {
    if (length < 0) {
        return str.mid(start);
    }
    return str.mid(start, length);
}

} // namespace string
} // namespace utils
} // namespace sc