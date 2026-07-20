/**
 * @file namedpipe/named_pipe_adapter.h
 * @brief 命名管道适配器
 * @details Windows 命名管道的适配器实现
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_NAMEDPIPE_NAMED_PIPE_ADAPTER_H
#define SOUL_NETWORK_NAMEDPIPE_NAMED_PIPE_ADAPTER_H

#include "soul/network/core/network_adapter_base.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT NamedPipeAdapter : public NetworkAdapterBase {
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

} // namespace network
} // namespace sc

#endif