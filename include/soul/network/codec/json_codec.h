/**
 * @file codec/json_codec.h
 * @brief JSON 编解码器
 * @details 实现 JSON 数据的序列化和反序列化
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_CODEC_JSON_CODEC_H
#define SOUL_NETWORK_CODEC_JSON_CODEC_H

#include "soul/network/network_global.h"
#include "icodec.h"
#include <QJsonDocument>

namespace sc {
namespace network {

class SC_NETWORK_EXPORT JsonCodec : public ICodec {
public:
    QByteArray encode(const NetworkMessage& message) override;
    NetworkMessage decode(const QByteArray& data) override;
    QString contentType() const override;
    
    std::string interfaceName() const override;

    void setCompact(bool compact);
    bool isCompact() const;

private:
    bool m_compact = true;
};

} // namespace network
} // namespace sc

#endif