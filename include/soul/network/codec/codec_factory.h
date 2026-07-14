#ifndef SOUL_NETWORK_CODEC_CODEC_FACTORY_H
#define SOUL_NETWORK_CODEC_CODEC_FACTORY_H

#include <memory>
#include <QString>
#include "soul/core/singleton.h"
#include "icodec.h"

namespace sc::network {

class CodecFactory : public Singleton<CodecFactory> {
public:
    enum class CodecType {
        JSON,
        XML,
        Protobuf,
        MessagePack,
        FlatBuffer
    };

    std::shared_ptr<ICodec> create(CodecType type);
    std::shared_ptr<ICodec> create(const QString& contentType);
};

}

#endif