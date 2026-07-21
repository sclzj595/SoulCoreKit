#include "soul/network/web_socket.h"
#include <QTimer>
#include "soul/logging/logger.h"

namespace sc {
namespace network {

WebSocket::WebSocket(QObject* parent) : QObject(parent) {
    m_socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    connect(m_socket, &QWebSocket::connected, this, &WebSocket::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &WebSocket::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocket::onTextMessageReceived);
    connect(m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocket::onBinaryMessageReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &WebSocket::onError);

    m_stateMachine.setStateChangedCallback([this](ConnectionState oldState, ConnectionState newState) {
        Logger::instance().info("WebSocket state changed: " + m_stateMachine.stateToString(oldState) + " -> " +
                               m_stateMachine.stateToString(newState), "WebSocket");
    });
}

WebSocket::~WebSocket() {
    disconnectFromServer();
}

void WebSocket::connectToServer(const QUrl& url) {
    if (!m_stateMachine.canConnect()) {
        Logger::instance().warn("Cannot connect, current state: " + m_stateMachine.stateToString(m_stateMachine.state()), "WebSocket");
        return;
    }

    m_url = url;
    m_stateMachine.transitionToConnecting();
    m_socket->open(url);
}

void WebSocket::disconnectFromServer() {
    if (!m_stateMachine.canDisconnect()) {
        return;
    }

    m_stateMachine.transitionToDisconnecting();
    m_socket->close();
    m_stateMachine.transitionToDisconnected();
}

void WebSocket::sendTextMessage(const QString& message) {
    if (!m_stateMachine.canSend()) {
        Logger::instance().warn("Cannot send text, current state: " + m_stateMachine.stateToString(m_stateMachine.state()), "WebSocket");
        return;
    }
    m_socket->sendTextMessage(message);
}

void WebSocket::sendBinaryMessage(const QByteArray& data) {
    if (!m_stateMachine.canSend()) {
        Logger::instance().warn("Cannot send binary, current state: " + m_stateMachine.stateToString(m_stateMachine.state()), "WebSocket");
        return;
    }
    m_socket->sendBinaryMessage(data);
}

bool WebSocket::isConnected() const {
    return m_stateMachine.isConnected();
}

ConnectionState WebSocket::state() const {
    return m_stateMachine.state();
}

void WebSocket::setMessageCallback(MessageCallback callback) {
    m_messageCallback = callback;
}

void WebSocket::setBinaryCallback(BinaryCallback callback) {
    m_binaryCallback = callback;
}

void WebSocket::setConnectedCallback(ConnectedCallback callback) {
    m_connectedCallback = callback;
}

void WebSocket::setDisconnectedCallback(DisconnectedCallback callback) {
    m_disconnectedCallback = callback;
}

void WebSocket::setErrorCallback(ErrorCallback callback) {
    m_errorCallback = callback;
}

void WebSocket::setStateChangedCallback(StateChangedCallback callback) {
    m_stateMachine.setStateChangedCallback(callback);
}

void WebSocket::setReconnectPolicy(const ReconnectPolicy& policy) {
    m_reconnectPolicy = policy;
}

ReconnectPolicy WebSocket::reconnectPolicy() const {
    return m_reconnectPolicy;
}

void WebSocket::onConnected() {
    m_reconnectPolicy.resetRetry();
    m_stateMachine.transitionToConnected();
    if (m_connectedCallback) {
        m_connectedCallback();
    }
}

void WebSocket::onDisconnected() {
    bool wasConnected = m_stateMachine.isConnected() || m_stateMachine.isConnecting();
    m_stateMachine.transitionToDisconnected();
    if (m_disconnectedCallback) {
        m_disconnectedCallback();
    }
    if (wasConnected && m_reconnectPolicy.shouldReconnect()) {
        m_reconnectPolicy.incrementRetry();
        QTimer::singleShot(m_reconnectPolicy.nextRetryInterval().count(), this, &WebSocket::reconnect);
    }
}

void WebSocket::onTextMessageReceived(const QString& message) {
    if (m_messageCallback) {
        m_messageCallback(message);
    }
}

void WebSocket::onBinaryMessageReceived(const QByteArray& data) {
    if (m_binaryCallback) {
        m_binaryCallback(data);
    }
}

void WebSocket::onError(QAbstractSocket::SocketError error) {
    Logger::instance().error("WebSocket error: " + QString::number(static_cast<int>(error)).toStdString(), "WebSocket");
    m_stateMachine.transitionToError();
    if (m_errorCallback) {
        m_errorCallback(error);
    }
}

void WebSocket::reconnect() {
    if (m_stateMachine.canConnect()) {
        m_stateMachine.transitionToConnecting();
        m_socket->open(m_url);
    }
}

} // namespace network
} // namespace sc