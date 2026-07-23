#include "soul/storage/json_serializer.h"
#include "soul/core/error.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace sc {

QString JsonSerializer::name() const {
    return "json";
}

Result<QByteArray> JsonSerializer::serialize(const QVariant& data) const {
    QJsonDocument doc;

    if (data.canConvert<QJsonObject>()) {
        doc = QJsonDocument(data.value<QJsonObject>());
    } else if (data.canConvert<QJsonArray>()) {
        doc = QJsonDocument(data.value<QJsonArray>());
    } else if (data.canConvert<QJsonValue>()) {
        QJsonValue value = data.value<QJsonValue>();
        if (value.isObject()) {
            doc = QJsonDocument(value.toObject());
        } else if (value.isArray()) {
            doc = QJsonDocument(value.toArray());
        } else {
            return Error(ErrorCode::SerializationError, "Unsupported QVariant type");
        }
    } else {
        return Error(ErrorCode::SerializationError, "Unsupported QVariant type");
    }

    return doc.toJson(m_format);
}

Result<QVariant> JsonSerializer::deserialize(const QByteArray& data) const {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        return Error(ErrorCode::DeserializationError, error.errorString().toStdString());
    }

    if (doc.isObject()) {
        return QVariant::fromValue(doc.object());
    } else if (doc.isArray()) {
        return QVariant::fromValue(doc.array());
    }

    return Error(ErrorCode::DeserializationError, "Invalid JSON document");
}

}
