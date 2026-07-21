#ifndef SOUL_NETWORK_WEB_SOCKET_H
#define SOUL_NETWORK_WEB_SOCKET_H

#include <QObject>
#include <QWebSocket>
#include <functional>
#include <memory>
#include "soul/network/network_global.h"
#include "soul/network/policy/reconnect_policy.h"
#include "soul/network/core/connection_state_machine.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT WebSocket : public QObject {
    Q_OBJECT
public:
    using MessageCallback = std::function<void(const QString& message)>;
    using BinaryCallback = std::function<void(const QByteArray& data)>;
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;
    using ErrorCallback = std::function<void(QAbstractSocket::SocketError error)>;
    using StateChangedCallback = std::function<void(ConnectionState oldState, ConnectionState newState)>;

    explicit WebSocket(QObject* parent = nullptr);
    ~WebSocket();

    void connectToServer(const QUrl& url);
    void disconnectFromServer();
    void sendTextMessage(const QString& message);
    void sendBinaryMessage(const QByteArray& data);
    bool isConnected() const;
    ConnectionState state() const;

    void setMessageCallback(MessageCallback callback);
    void setBinaryCallback(BinaryCallback callback);
    void setConnectedCallback(ConnectedCallback callback);
    void setDisconnectedCallback(DisconnectedCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setStateChangedCallback(StateChangedCallback callback);

    void setReconnectPolicy(const ReconnectPolicy& policy);
    ReconnectPolicy reconnectPolicy() const;

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& data);
    void onError(QAbstractSocket::SocketError error);
    void reconnect();

private:
    QWebSocket* m_socket = nullptr;
    QUrl m_url;

    MessageCallback m_messageCallback;
    BinaryCallback m_binaryCallback;
    ConnectedCallback m_connectedCallback;
    DisconnectedCallback m_disconnectedCallback;
    ErrorCallback m_errorCallback;

    ReconnectPolicy m_reconnectPolicy;
    ConnectionStateMachine m_stateMachine;
};

} // namespace network
} // namespace sc

#endif