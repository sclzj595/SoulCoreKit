/**
 * @file websocket/ws_client_adapter.h
 * @brief WebSocket 客户端适配器
 * @details WebSocket 协议的适配器实现
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_WEBSOCKET_WS_CLIENT_ADAPTER_H
#define SOUL_NETWORK_WEBSOCKET_WS_CLIENT_ADAPTER_H

#include <memory>
#include "soul/network/core/network_adapter_base.h"
#include "soul/network/web_socket.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT WsClientAdapter : public NetworkAdapterBase {
    Q_OBJECT
public:
    explicit WsClientAdapter(QObject* parent = nullptr);
    ~WsClientAdapter() override;

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& data);

private:
    Result<NetworkMessage> doSend(const NetworkMessage& message) override;
    void doSendAsync(const NetworkMessage& message, ResponseCallback callback) override;
    void doConnect(const QUrl& url) override;
    void doDisconnect() override;
    bool doIsConnected() const override;

    std::unique_ptr<WebSocket> m_client;
};

} // namespace network
} // namespace sc

#endif