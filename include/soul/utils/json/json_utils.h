#ifndef SOUL_UTILS_JSON_UTILS_H
#define SOUL_UTILS_JSON_UTILS_H

// ============================================================================
// json_utils.h — 已废弃,请使用 json_helper.h (v1.9.2)
// ============================================================================
// 本文件保留向后兼容,所有函数委托到 sc::json 命名空间。
// 新代码请直接使用 #include "soul/utils/json/json_helper.h"

#include "soul/utils/json/json_helper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

namespace sc::utils::json {

// 文件 I/O (委托到 sc::json)
inline QJsonDocument fromFile(const QString& filePath) {
    auto result = sc::json::loadFromFile(filePath);
    if (result.isOk()) {
        return sc::json::toQJsonDocument(result.unwrap());
    }
    return QJsonDocument();
}

inline bool toFile(const QJsonDocument& doc, const QString& filePath) {
    auto j = sc::json::fromQJsonDocument(doc);
    return sc::json::saveToFile(j, filePath).isOk();
}

// 安全取值 (委托到 sc::json)
inline QString getString(const QJsonObject& obj, const QString& key, const QString& defaultValue = "") {
    auto j = sc::json::fromQJsonObject(obj);
    return sc::json::getString(j, key.toStdString(), defaultValue);
}

inline int getInt(const QJsonObject& obj, const QString& key, int defaultValue = 0) {
    auto j = sc::json::fromQJsonObject(obj);
    return sc::json::getInt(j, key.toStdString(), defaultValue);
}

inline double getDouble(const QJsonObject& obj, const QString& key, double defaultValue = 0.0) {
    auto j = sc::json::fromQJsonObject(obj);
    return sc::json::getDouble(j, key.toStdString(), defaultValue);
}

inline bool getBool(const QJsonObject& obj, const QString& key, bool defaultValue = false) {
    auto j = sc::json::fromQJsonObject(obj);
    return sc::json::getBool(j, key.toStdString(), defaultValue);
}

inline QJsonArray getArray(const QJsonObject& obj, const QString& key) {
    auto j = sc::json::fromQJsonObject(obj);
    return sc::json::toQJsonArray(sc::json::getArray(j, key.toStdString()));
}

inline QJsonObject getObject(const QJsonObject& obj, const QString& key) {
    auto j = sc::json::fromQJsonObject(obj);
    return sc::json::toQJsonObject(sc::json::getObject(j, key.toStdString()));
}

inline bool contains(const QJsonObject& obj, const QString& key) {
    return obj.contains(key);
}

inline QString toPrettyString(const QJsonDocument& doc) {
    auto j = sc::json::fromQJsonDocument(doc);
    return QString::fromUtf8(sc::json::serializePretty(j));
}

inline QString toCompactString(const QJsonDocument& doc) {
    auto j = sc::json::fromQJsonDocument(doc);
    return QString::fromUtf8(sc::json::serialize(j));
}

template<typename T>
QJsonArray toJsonArray(const std::vector<T>& items);

template<typename T>
std::vector<T> fromJsonArray(const QJsonArray& array);

}

#endif