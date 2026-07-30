#ifndef SOUL_UTILS_JSON_HELPER_H
#define SOUL_UTILS_JSON_HELPER_H

// ============================================================================
// json_helper.h — nlohmann/json 全局工具封装 (v1.9.2)
// ============================================================================
//
// 设计目标: 统一使用 nlohmann/json 处理所有非 UI 模块的 JSON 序列化。
// Qt JSON (QJsonDocument/QJsonObject) 仅保留用于 UI 临时数据序列化。
//
// 设计原则:
//   - 单一入口: 所有模块通过 sc::json 命名空间使用 JSON
//   - 类型安全: 编译期类型检查,避免字符串 key 拼写错误
//   - Qt 互操作: 提供 nlohmann::json ↔ Qt 类型的双向转换
//   - 错误处理: 使用 Result<T> 模式,失败时返回明确错误信息
//
// 用法:
//   #include "soul/utils/json/json_helper.h"
//   using namespace sc::json;
//
//   // 序列化
//   nlohmann::json j;
//   j["name"] = "Alice";
//   j["age"] = 30;
//   QByteArray data = serialize(j);
//
//   // 反序列化
//   auto result = deserialize(data);
//   if (result.isOk()) {
//       nlohmann::json j = result.unwrap();
//       QString name = j["name"].get<QString>();
//   }
//
//   // Qt 类型转换
//   QJsonObject qobj = toQJsonObject(j);
//   nlohmann::json j2 = fromQJsonObject(qobj);

#include <nlohmann/json.hpp>

#include <QString>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>

#include "soul/core/result.h"

namespace sc {
namespace json {

// ============================================================================
// 类型别名
// ============================================================================
using Json = nlohmann::json;

// ============================================================================
// 序列化 / 反序列化
// ============================================================================

/// @brief 将 nlohmann::json 序列化为 QByteArray (UTF-8)
inline QByteArray serialize(const Json& j) {
    return QByteArray::fromStdString(j.dump());
}

/// @brief 将 nlohmann::json 序列化为格式化 QByteArray (缩进 2 空格)
inline QByteArray serializePretty(const Json& j) {
    return QByteArray::fromStdString(j.dump(2));
}

/// @brief 从 QByteArray 反序列化为 nlohmann::json
/// @return Result<Json>,失败时包含错误信息
inline Result<Json> deserialize(const QByteArray& data) {
    try {
        return Result<Json>(Json::parse(data.toStdString()));
    } catch (const Json::parse_error& e) {
        return Result<Json>(Error(ErrorCode::DeserializationError,
            QString::fromStdString(e.what())));
    }
}

/// @brief 从 QByteArray 反序列化,接受非严格 JSON (如注释、尾逗号)
inline Result<Json> deserializeLenient(const QByteArray& data) {
    try {
        return Result<Json>(Json::parse(data.toStdString(), nullptr, true, true));
    } catch (const Json::parse_error& e) {
        return Result<Json>(Error(ErrorCode::DeserializationError,
            QString::fromStdString(e.what())));
    }
}

// ============================================================================
// 文件 I/O
// ============================================================================

/// @brief 从文件读取并解析 JSON
inline Result<Json> loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<Json>(Error(ErrorCode::FileNotFound,
            QString("Cannot open file: %1").arg(filePath)));
    }
    return deserialize(file.readAll());
}

/// @brief 将 JSON 写入文件 (紧凑格式)
inline Result<void> saveToFile(const Json& j, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result<void>(Error(ErrorCode::FileWriteError,
            QString("Cannot write file: %1").arg(filePath)));
    }
    file.write(serialize(j));
    return {};
}

/// @brief 将 JSON 写入文件 (格式化,缩进 2 空格)
inline Result<void> saveToFilePretty(const Json& j, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result<void>(Error(ErrorCode::FileWriteError,
            QString("Cannot write file: %1").arg(filePath)));
    }
    file.write(serializePretty(j));
    return {};
}

// ============================================================================
// nlohmann::json ↔ QJsonObject 转换
// ============================================================================

/// @brief QJsonValue → nlohmann::json
inline Json fromQJsonValue(const QJsonValue& value) {
    switch (value.type()) {
    case QJsonValue::Null:   return nullptr;
    case QJsonValue::Bool:   return value.toBool();
    case QJsonValue::Double: return value.toDouble();
    case QJsonValue::String: return value.toString().toStdString();
    case QJsonValue::Array: {
        Json arr = Json::array();
        const QJsonArray qarr = value.toArray();
        for (const auto& v : qarr) {
            arr.push_back(fromQJsonValue(v));
        }
        return arr;
    }
    case QJsonValue::Object: {
        Json obj = Json::object();
        const QJsonObject qobj = value.toObject();
        for (auto it = qobj.begin(); it != qobj.end(); ++it) {
            obj[it.key().toStdString()] = fromQJsonValue(it.value());
        }
        return obj;
    }
    default: return nullptr;
    }
}

/// @brief QJsonObject → nlohmann::json
inline Json fromQJsonObject(const QJsonObject& qobj) {
    return fromQJsonValue(QJsonValue(qobj));
}

/// @brief QJsonArray → nlohmann::json
inline Json fromQJsonArray(const QJsonArray& qarr) {
    return fromQJsonValue(QJsonValue(qarr));
}

/// @brief QJsonDocument → nlohmann::json
inline Json fromQJsonDocument(const QJsonDocument& doc) {
    if (doc.isObject()) return fromQJsonObject(doc.object());
    if (doc.isArray())  return fromQJsonArray(doc.array());
    return nullptr;
}

/// @brief nlohmann::json → QJsonValue
inline QJsonValue toQJsonValue(const Json& j) {
    switch (j.type()) {
    case Json::value_t::null:    return QJsonValue();
    case Json::value_t::boolean: return QJsonValue(j.get<bool>());
    case Json::value_t::number_integer:
    case Json::value_t::number_unsigned: return QJsonValue(static_cast<qint64>(j.get<int64_t>()));
    case Json::value_t::number_float:   return QJsonValue(j.get<double>());
    case Json::value_t::string:  return QString::fromStdString(j.get<std::string>());
    case Json::value_t::array: {
        QJsonArray arr;
        for (const auto& v : j) {
            arr.append(toQJsonValue(v));
        }
        return arr;
    }
    case Json::value_t::object: {
        QJsonObject obj;
        for (auto it = j.begin(); it != j.end(); ++it) {
            obj[QString::fromStdString(it.key())] = toQJsonValue(it.value());
        }
        return obj;
    }
    default: return QJsonValue();
    }
}

/// @brief nlohmann::json → QJsonObject
inline QJsonObject toQJsonObject(const Json& j) {
    if (!j.is_object()) return QJsonObject();
    return toQJsonValue(j).toObject();
}

/// @brief nlohmann::json → QJsonArray
inline QJsonArray toQJsonArray(const Json& j) {
    if (!j.is_array()) return QJsonArray();
    return toQJsonValue(j).toArray();
}

/// @brief nlohmann::json → QJsonDocument
inline QJsonDocument toQJsonDocument(const Json& j) {
    if (j.is_object()) return QJsonDocument(toQJsonObject(j));
    if (j.is_array())  return QJsonDocument(toQJsonArray(j));
    return QJsonDocument();
}

// ============================================================================
// 安全取值辅助 (带默认值,避免异常)
// ============================================================================

/// @brief 安全获取字符串值
inline QString getString(const Json& j, const std::string& key, const QString& defaultValue = {}) {
    auto it = j.find(key);
    if (it != j.end() && it->is_string()) {
        return QString::fromStdString(it->get<std::string>());
    }
    return defaultValue;
}

/// @brief 安全获取整数值 (同时接受有符号和无符号整数)
inline int64_t getInt64(const Json& j, const std::string& key, int64_t defaultValue = 0) {
    auto it = j.find(key);
    if (it != j.end() && (it->is_number_integer() || it->is_number_unsigned())) {
        return it->get<int64_t>();
    }
    return defaultValue;
}

/// @brief 安全获取 int 值 (同时接受有符号和无符号整数)
inline int getInt(const Json& j, const std::string& key, int defaultValue = 0) {
    auto it = j.find(key);
    if (it != j.end() && (it->is_number_integer() || it->is_number_unsigned())) {
        return it->get<int>();
    }
    return defaultValue;
}

/// @brief 安全获取 double 值
inline double getDouble(const Json& j, const std::string& key, double defaultValue = 0.0) {
    auto it = j.find(key);
    if (it != j.end() && it->is_number()) {
        return it->get<double>();
    }
    return defaultValue;
}

/// @brief 安全获取 bool 值
inline bool getBool(const Json& j, const std::string& key, bool defaultValue = false) {
    auto it = j.find(key);
    if (it != j.end() && it->is_boolean()) {
        return it->get<bool>();
    }
    return defaultValue;
}

/// @brief 安全获取嵌套 JSON 对象
inline Json getObject(const Json& j, const std::string& key) {
    auto it = j.find(key);
    if (it != j.end() && it->is_object()) {
        return *it;
    }
    return Json::object();
}

/// @brief 安全获取 JSON 数组
inline Json getArray(const Json& j, const std::string& key) {
    auto it = j.find(key);
    if (it != j.end() && it->is_array()) {
        return *it;
    }
    return Json::array();
}

/// @brief 检查 key 是否存在
inline bool contains(const Json& j, const std::string& key) {
    return j.contains(key);
}

// ============================================================================
// Qt 类型直接转换
// ============================================================================

/// @brief QString → nlohmann::json
inline Json fromQString(const QString& s) {
    return Json(s.toStdString());
}

/// @brief nlohmann::json → QString (要求值是 string 类型)
inline QString toQString(const Json& j) {
    if (j.is_string()) return QString::fromStdString(j.get<std::string>());
    return QString::fromStdString(j.dump());
}

} // namespace json
} // namespace sc

#endif // SOUL_UTILS_JSON_HELPER_H