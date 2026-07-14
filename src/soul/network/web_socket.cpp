#include "soul/network/web_socket.h"
#include <QTimer>
#include "soul/logging/logger.h"

namespace sc {

WebSocket::WebSocket(QObject* parent) : QObject(parent) {
    m_socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    connect(m_socket, &QWebSocket::connected, this, &WebSocket::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &WebSocket::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocket::onTextMessageReceived);
    connect(m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocket::onBinaryMessageReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &WebSocket::onError);
}

WebSocket::~WebSocket() {}

void WebSocket::connectToServer(const QUrl& url) {
    m_url = url;
    m_socket->open(url);
}

void WebSocket::disconnectFromServer() {
    m_socket->close();
}

void WebSocket::sendTextMessage(const QString& message) {
    if (isConnected()) {
        m_socket->sendTextMessage(message);
    }
}

void WebSocket::sendBinaryMessage(const QByteArray& data) {
    if (isConnected()) {
        m_socket->sendBinaryMessage(data);
    }
}

bool WebSocket::isConnected() const {
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
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

void WebSocket::setReconnectPolicy(const network::ReconnectPolicy& policy) {
    m_reconnectPolicy = policy;
}

network::ReconnectPolicy WebSocket::reconnectPolicy() const {
    return m_reconnectPolicy;
}

void WebSocket::onConnected() {
    m_reconnectPolicy.resetRetry();
    if (m_connectedCallback) {
        m_connectedCallback();
    }
}

void WebSocket::onDisconnected() {
    if (m_disconnectedCallback) {
        m_disconnectedCallback();
    }
    if (m_reconnectPolicy.shouldReconnect()) {
        m_reconnectPolicy.incrementRetry();
        QTimer::singleShot(m_reconnectPolicy.interval.count(), this, &WebSocket::reconnect);
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
    if (m_errorCallback) {
        m_errorCallback(error);
    }
}

void WebSocket::reconnect() {
    if (!isConnected()) {
        m_socket->open(m_url);
    }
}

}