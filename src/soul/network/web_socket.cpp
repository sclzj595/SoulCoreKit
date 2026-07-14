#include "soul/network/web_socket.h"
#include <QTimer>

namespace sc {

WebSocket::WebSocket(QObject* parent) : QObject(parent) {
    m_socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
}

WebSocket::~WebSocket() {}

void WebSocket::connectToServer(const QUrl& url) {
    m_url = url;
    m_socket->open(url);

    connect(m_socket, &QWebSocket::connected, this, &WebSocket::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &WebSocket::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocket::onTextMessageReceived);
    connect(m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocket::onBinaryMessageReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &WebSocket::onError);
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

void WebSocket::setAutoReconnect(bool enabled) {
    m_autoReconnect = enabled;
}

bool WebSocket::autoReconnect() const {
    return m_autoReconnect;
}

void WebSocket::setReconnectInterval(int ms) {
    m_reconnectInterval = ms;
}

int WebSocket::reconnectInterval() const {
    return m_reconnectInterval;
}

void WebSocket::onConnected() {
    if (m_connectedCallback) {
        m_connectedCallback();
    }
}

void WebSocket::onDisconnected() {
    if (m_disconnectedCallback) {
        m_disconnectedCallback();
    }
    if (m_autoReconnect) {
        QTimer::singleShot(m_reconnectInterval, this, &WebSocket::reconnect);
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
    Q_UNUSED(error);
}

void WebSocket::reconnect() {
    if (!isConnected()) {
        m_socket->open(m_url);
    }
}

}