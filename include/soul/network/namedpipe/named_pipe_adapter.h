#ifndef SOUL_NETWORK_NAMEDPIPE_NAMED_PIPE_ADAPTER_H
#define SOUL_NETWORK_NAMEDPIPE_NAMED_PIPE_ADAPTER_H

#include "soul/network/core/network_adapter_base.h"

namespace sc::network {

class NamedPipeAdapter : public NetworkAdapterBase {
    Q_OBJECT
public:
    NamedPipeAdapter();
    
protected:
    Result<NetworkMessage> doSend(const NetworkMessage& message) override;
    void doSendAsync(const NetworkMessage& message, ResponseCallback callback) override;
    void doConnect(const QUrl& url) override;
    void doDisconnect() override;
    bool doIsConnected() const override;
    
private:
    bool m_connected = false;
};

}

#endif