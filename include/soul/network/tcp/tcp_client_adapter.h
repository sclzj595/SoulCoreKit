#ifndef SOUL_NETWORK_TCP_TCP_CLIENT_ADAPTER_H
#define SOUL_NETWORK_TCP_TCP_CLIENT_ADAPTER_H

#include <memory>
#include "soul/network/core/network_adapter_base.h"
#include "soul/network/tcp_client.h"

namespace sc::network {

class TcpClientAdapter : public NetworkAdapterBase {
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

}

#endif