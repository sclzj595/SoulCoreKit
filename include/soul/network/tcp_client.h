#ifndef SOUL_NETWORK_TCP_CLIENT_H
#define SOUL_NETWORK_TCP_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <functional>
#include "soul/network/network_global.h"
#include "soul/network/policy/reconnect_policy.h"
#include "soul/network/core/connection_state_machine.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT TcpClient : public QObject {
    Q_OBJECT
public:
    using DataCallback = std::function<void(const QByteArray& data)>;
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;
    using ErrorCallback = std::function<void(QAbstractSocket::SocketError error)>;
    using StateChangedCallback = std::function<void(ConnectionState oldState, ConnectionState newState)>;

    explicit TcpClient(QObject* parent = nullptr);
    ~TcpClient();

    void connectToHost(const QString& host, quint16 port);
    void disconnectFromHost();
    void send(const QByteArray& data);
    bool isConnected() const;
    ConnectionState state() const;

    void setDataCallback(DataCallback callback);
    void setConnectedCallback(ConnectedCallback callback);
    void setDisconnectedCallback(DisconnectedCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setStateChangedCallback(StateChangedCallback callback);

    void setReconnectPolicy(const ReconnectPolicy& policy);
    ReconnectPolicy reconnectPolicy() const;

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void reconnect();

private:
    QTcpSocket* m_socket = nullptr;
    QString m_host;
    quint16 m_port = 0;

    DataCallback m_dataCallback;
    ConnectedCallback m_connectedCallback;
    DisconnectedCallback m_disconnectedCallback;
    ErrorCallback m_errorCallback;

    ReconnectPolicy m_reconnectPolicy;
    ConnectionStateMachine m_stateMachine;
};

} // namespace network
} // namespace sc

#endif