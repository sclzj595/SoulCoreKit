#include "soul/storage/json_serializer.h"
#include "soul/core/error.h"

namespace sc {

using namespace sc::json;

QString JsonSerializer::name() const {
    return "json";
}

Result<QByteArray> JsonSerializer::serialize(const QVariant& data) const {
    Json j;

    if (data.canConvert<QJsonObject>()) {
        j = fromQJsonObject(data.value<QJsonObject>());
    } else if (data.canConvert<QJsonArray>()) {
        j = fromQJsonArray(data.value<QJsonArray>());
    } else if (data.canConvert<QJsonValue>()) {
        QJsonValue value = data.value<QJsonValue>();
        if (value.isObject()) {
            j = fromQJsonObject(value.toObject());
        } else if (value.isArray()) {
            j = fromQJsonArray(value.toArray());
        } else {
            j = fromQJsonValue(value);
        }
    } else {
        return Error(ErrorCode::SerializationError, "Unsupported QVariant type");
    }

    return m_compact ? sc::json::serialize(j) : sc::json::serializePretty(j);
}

Result<QVariant> JsonSerializer::deserialize(const QByteArray& data) const {
    auto result = sc::json::deserialize(data);
    if (!result.isOk()) {
        return Error(ErrorCode::DeserializationError, "Failed to parse JSON");
    }

    Json j = result.unwrap();

    if (j.is_object()) {
        return QVariant::fromValue(toQJsonObject(j));
    } else if (j.is_array()) {
        return QVariant::fromValue(toQJsonArray(j));
    }

    return Error(ErrorCode::DeserializationError, "Invalid JSON document");
}

}
