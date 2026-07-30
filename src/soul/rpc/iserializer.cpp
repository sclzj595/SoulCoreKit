#include "soul/rpc/iserializer.h"
#include <type_traits>

namespace sc {
namespace rpc {

QByteArray JsonSerializer::serialize(const sc::json::Json& obj) const {
    return sc::json::serialize(obj);
}

Result<sc::json::Json> JsonSerializer::deserialize(const QByteArray& data) const {
    return sc::json::deserialize(data);
}

QByteArray JsonSerializer::serializeValue(const RpcValue& value) const {
    sc::json::Json obj = sc::json::Json::object();
    std::visit([&obj](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            obj["type"] = "null";
            obj["value"] = nullptr;
        } else if constexpr (std::is_same_v<T, qint64>) {
            obj["type"] = "int64";
            obj["value"] = val;
        } else if constexpr (std::is_same_v<T, double>) {
            obj["type"] = "double";
            obj["value"] = val;
        } else if constexpr (std::is_same_v<T, bool>) {
            obj["type"] = "bool";
            obj["value"] = val;
        } else if constexpr (std::is_same_v<T, QString>) {
            obj["type"] = "string";
            obj["value"] = val.toStdString();
        } else if constexpr (std::is_same_v<T, QByteArray>) {
            obj["type"] = "bytes";
            obj["value"] = val.toBase64().toStdString();
        }
    }, value);
    return sc::json::serialize(obj);
}

Result<RpcValue> JsonSerializer::deserializeValue(const QByteArray& data) const {
    auto result = sc::json::deserialize(data);
    if (result.isErr()) {
        return Result<RpcValue>(result.unwrapErr());
    }
    sc::json::Json j = result.unwrap();
    if (!j.is_object()) {
        return Result<RpcValue>(Error(ErrorCode::DeserializationError,
            "Expected JSON object for RpcValue"));
    }

    std::string type = j.value("type", "");
    auto& val = j["value"];

    if (type == "null") {
        return Result<RpcValue>(RpcValue{});
    } else if (type == "int64") {
        return Result<RpcValue>(RpcValue(static_cast<qint64>(val.get<int64_t>())));
    } else if (type == "double") {
        return Result<RpcValue>(RpcValue(val.get<double>()));
    } else if (type == "bool") {
        return Result<RpcValue>(RpcValue(val.get<bool>()));
    } else if (type == "string") {
        return Result<RpcValue>(RpcValue(QString::fromStdString(val.get<std::string>())));
    } else if (type == "bytes") {
        QByteArray decoded = QByteArray::fromBase64(
            QByteArray::fromStdString(val.get<std::string>()));
        return Result<RpcValue>(RpcValue(decoded));
    }
    return Result<RpcValue>(Error(ErrorCode::DeserializationError,
        QString("Unknown RpcValue type: %1").arg(QString::fromStdString(type))));
}

} // namespace rpc
} // namespace sc