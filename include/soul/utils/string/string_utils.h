#ifndef SOUL_UTILS_STRING_STRING_UTILS_H
#define SOUL_UTILS_STRING_STRING_UTILS_H

#include <QString>
#include <vector>

namespace sc::utils::string {

QString trim(const QString& str);
QString trimLeft(const QString& str);
QString trimRight(const QString& str);

QString toLower(const QString& str);
QString toUpper(const QString& str);

bool startsWith(const QString& str, const QString& prefix);
bool endsWith(const QString& str, const QString& suffix);
bool contains(const QString& str, const QString& substring);

std::vector<QString> split(const QString& str, const QString& delimiter);
QString join(const std::vector<QString>& parts, const QString& delimiter);

QString replace(const QString& str, const QString& oldValue, const QString& newValue);

QString format(const QString& format, ...);

bool isEmpty(const QString& str);
bool isBlank(const QString& str);

int count(const QString& str, const QString& substring);

QString substring(const QString& str, int start, int length = -1);

}

#endif