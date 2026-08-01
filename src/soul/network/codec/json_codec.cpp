#include <string>
#include "soul/network/codec/json_codec.h"

namespace sc {
namespace network {

using namespace sc::json;

QByteArray JsonCodec::encode(const NetworkMessage& message) {
    Json obj;
    obj["api"] = message.api.toStdString();
    obj["body"] = QString::fromUtf8(message.payload).toStdString();

    Json metaObj = Json::object();
    for (auto it = message.metadata.begin(); it != message.metadata.end(); ++it) {
        metaObj[it.key().toStdString()] = fromQJsonValue(QJsonValue::fromVariant(it.value()));
    }
    obj["metadata"] = metaObj;

    return m_compact ? serialize(obj) : serializePretty(obj);
}

NetworkMessage JsonCodec::decode(const QByteArray& data) {
    auto result = deserialize(data);
    if (!result.isOk() || !result.unwrap().is_object()) {
        return NetworkMessage();
    }

    Json obj = result.unwrap();
    NetworkMessage message;
    message.api = getString(obj, "api");
    message.payload = getString(obj, "body").toUtf8();

    if (contains(obj, "metadata")) {
        Json metaObj = obj["metadata"];
        if (metaObj.is_object()) {
            for (auto it = metaObj.begin(); it != metaObj.end(); ++it) {
                message.metadata[QString::fromStdString(it.key())] = toQJsonValue(it.value()).toVariant();
            }
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