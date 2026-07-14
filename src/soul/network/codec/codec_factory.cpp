#include "soul/network/codec/codec_factory.h"
#include "soul/network/codec/json_codec.h"

namespace sc::network {

std::shared_ptr<ICodec> CodecFactory::create(CodecType type) {
    switch (type) {
    case CodecType::JSON:
        return std::make_shared<JsonCodec>();
    case CodecType::XML:
    case CodecType::Protobuf:
    case CodecType::MessagePack:
    case CodecType::FlatBuffer:
        break;
    }
    return std::make_shared<JsonCodec>();
}

std::shared_ptr<ICodec> CodecFactory::create(const QString& contentType) {
    QString ct = contentType.toLower();
    if (ct.contains("json")) {
        return std::make_shared<JsonCodec>();
    }
    return std::make_shared<JsonCodec>();
}

}