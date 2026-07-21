#include "soul/network/codec/json_codec.h"
#include <QJsonObject>
#include <QJsonArray>

namespace sc {
namespace network {

QByteArray JsonCodec::encode(const NetworkMessage& message) {
    QJsonObject obj;
    obj["api"] = message.api;
    obj["body"] = QString::fromUtf8(message.payload);
    
    QJsonObject metaObj;
    for (auto it = message.metadata.begin(); it != message.metadata.end(); ++it) {
        QJsonValue value = QJsonValue::fromVariant(it.value());
        metaObj.insert(it.key(), value);
    }
    obj["metadata"] = metaObj;
    
    QJsonDocument doc(obj);
    return doc.toJson(m_compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

NetworkMessage JsonCodec::decode(const QByteArray& data) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        return NetworkMessage();
    }
    
    QJsonObject obj = doc.object();
    NetworkMessage message;
    message.api = obj["api"].toString();
    message.payload = obj["body"].toString().toUtf8();
    
    if (obj.contains("metadata")) {
        QJsonObject metaObj = obj["metadata"].toObject();
        for (auto it = metaObj.begin(); it != metaObj.end(); ++it) {
            message.metadata[it.key()] = it.value().toVariant();
        }
    }
    
    return message;
}

QString JsonCodec::contentType() const {
    return "application/json";
}

void JsonCodec::setCompact(bool compact) {
    m_compact = compact;
}

bool JsonCodec::isCompact() const {
    return m_compact;
}

std::string JsonCodec::interfaceName() const {
    return "JsonCodec";
}

} // namespace network
} // namespace sc