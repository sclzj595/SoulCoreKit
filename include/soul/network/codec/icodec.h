/**
 * @file codec/icodec.h
 * @brief 编解码器接口
 * @details 定义数据编解码的统一接口
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_CODEC_ICODEC_H
#define SOUL_NETWORK_CODEC_ICODEC_H

#include "soul/core/interface.h"
#include "soul/network/core/network_message.h"

namespace sc {
namespace network {

class ICodec : public IInterface {
public:
    ~ICodec() override = default;

    virtual QByteArray encode(const NetworkMessage& message) = 0;
    virtual NetworkMessage decode(const QByteArray& data) = 0;
    virtual QString contentType() const = 0;

    std::string interfaceName() const override {
        return "ICodec";
    }
};

} // namespace network
} // namespace sc

#endif