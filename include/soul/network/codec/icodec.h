#ifndef SOUL_NETWORK_CODEC_ICODEC_H
#define SOUL_NETWORK_CODEC_ICODEC_H

#include "soul/core/interface.h"
#include "soul/network/core/network_message.h"

namespace sc::network {

class ICodec : public IInterface {
public:
    ~ICodec() override = default;

    virtual QByteArray encode(const NetworkMessage& message) = 0;
    virtual NetworkMessage decode(const QByteArray& data) = 0;
    virtual QString contentType() const = 0;
};

}

#endif