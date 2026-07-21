/**
 * @file codec/codec_factory.h
 * @brief 编解码器工厂
 * @details 工厂模式，根据类型创建对应的编解码器
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_CODEC_CODEC_FACTORY_H
#define SOUL_NETWORK_CODEC_CODEC_FACTORY_H

#include <memory>
#include <QString>
#include "soul/core/singleton.h"
#include "icodec.h"

namespace sc {
namespace network {

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

} // namespace network
} // namespace sc

#endif