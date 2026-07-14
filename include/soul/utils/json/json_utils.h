#ifndef SOUL_UTILS_JSON_UTILS_H
#define SOUL_UTILS_JSON_UTILS_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

namespace sc::utils::json {

QJsonDocument fromFile(const QString& filePath);
bool toFile(const QJsonDocument& doc, const QString& filePath);

QString getString(const QJsonObject& obj, const QString& key, const QString& defaultValue = "");
int getInt(const QJsonObject& obj, const QString& key, int defaultValue = 0);
double getDouble(const QJsonObject& obj, const QString& key, double defaultValue = 0.0);
bool getBool(const QJsonObject& obj, const QString& key, bool defaultValue = false);
QJsonArray getArray(const QJsonObject& obj, const QString& key);
QJsonObject getObject(const QJsonObject& obj, const QString& key);

bool contains(const QJsonObject& obj, const QString& key);

QString toPrettyString(const QJsonDocument& doc);
QString toCompactString(const QJsonDocument& doc);

template<typename T>
QJsonArray toJsonArray(const std::vector<T>& items);

template<typename T>
std::vector<T> fromJsonArray(const QJsonArray& array);

}

#endif