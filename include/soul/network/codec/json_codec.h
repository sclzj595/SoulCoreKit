#ifndef SOUL_NETWORK_CODEC_JSON_CODEC_H
#define SOUL_NETWORK_CODEC_JSON_CODEC_H

#include "icodec.h"
#include <QJsonDocument>

namespace sc::network {

class JsonCodec : public ICodec {
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

}

#endif