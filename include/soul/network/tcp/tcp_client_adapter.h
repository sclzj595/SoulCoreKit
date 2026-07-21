/**
 * @file tcp/tcp_client_adapter.h
 * @brief TCP 客户端适配器
 * @details TCP 协议的适配器实现
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_TCP_TCP_CLIENT_ADAPTER_H
#define SOUL_NETWORK_TCP_TCP_CLIENT_ADAPTER_H

#include <memory>
#include "soul/network/core/network_adapter_base.h"
#include "soul/network/tcp_client.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT TcpClientAdapter : public NetworkAdapterBase {
    Q_OBJECT
public:
    explicit TcpClientAdapter(QObject* parent = nullptr);
    ~TcpClientAdapter() override;

private slots:
    void onConnected();
    void onDisconnected();
    void onDataReceived(const QByteArray& data);

private:
    Result<NetworkMessage> doSend(const NetworkMessage& message) override;
    void doSendAsync(const NetworkMessage& message, ResponseCallback callback) override;
    void doConnect(const QUrl& url) override;
    void doDisconnect() override;
    bool doIsConnected() const override;

    std::unique_ptr<TcpClient> m_client;
    quint32 m_sequence = 0;
};

} // namespace network
} // namespace sc

#endif