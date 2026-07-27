#include "soul/rpc/iserializer.h"
#include <QJsonDocument>
#include <QJsonValue>
#include <type_traits>

namespace sc {
namespace rpc {

QByteArray JsonSerializer::serialize(const QJsonObject& obj) const {
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

Result<QJsonObject> JsonSerializer::deserialize(const QByteArray& data) const {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        return Result<QJsonObject>(Error(ErrorCode::DeserializationError, error.errorString()));
    }
    if (!doc.isObject()) {
        return Result<QJsonObject>(Error(ErrorCode::DeserializationError, "Not a JSON object"));
    }
    return Result<QJsonObject>(doc.object());
}

QByteArray JsonSerializer::serializeValue(const RpcValue& value) const {
    QJsonObject obj;
    std::visit([&obj](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            obj["type"] = "null";
            obj["value"] = QJsonValue();
        } else if constexpr (std::is_same_v<T, qint64>) {
            obj["type"] = "int64";
            obj["value"] = QJsonValue(static_cast<double>(val));
        } else if constexpr (std::is_same_v<T, double>) {
            obj["type"] = "double";
            obj["value"] = QJsonValue(val);
        } else if constexpr (std::is_same_v<T, bool>) {
            obj["type"] = "bool";
            obj["value"] = QJsonValue(val);
        } else if constexpr (std::is_same_v<T, QString>) {
            obj["type"] = "string";
            obj["value"] = QJsonValue(val);
        } else if constexpr (std::is_same_v<T, QByteArray>) {
            obj["type"] = "bytes";
            obj["value"] = QJsonValue(QString::fromLatin1(val.toBase64()));
        }
    }, value);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

RpcValue JsonSerializer::deserializeValue(const QByteArray& data) const {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        return RpcValue{};
    }
    if (!doc.isObject()) {
        return RpcValue{};
    }
    QJsonObject obj = doc.object();
    QString type = obj.value("type").toString();
    QJsonValue val = obj.value("value");

    if (type == "null") {
        return RpcValue{};
    } else if (type == "int64") {
        return RpcValue(static_cast<qint64>(val.toDouble()));
    } else if (type == "double") {
        return RpcValue(val.toDouble());
    } else if (type == "bool") {
        return RpcValue(val.toBool());
    } else if (type == "string") {
        return RpcValue(val.toString());
    } else if (type == "bytes") {
        QByteArray decoded = QByteArray::fromBase64(val.toString().toLatin1());
        return RpcValue(decoded);
    }
    return RpcValue{};
}

} // namespace rpc
} // namespace sc