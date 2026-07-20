#include "soul/network/tcp_client.h"
#include <QTimer>
#include "soul/logging/logger.h"

namespace sc {
namespace network {

TcpClient::TcpClient(QObject* parent) : QObject(parent) {
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpClient::onError);

    m_stateMachine.setStateChangedCallback([this](ConnectionState oldState, ConnectionState newState) {
        Logger::instance().info("TCP state changed: " + m_stateMachine.stateToString(oldState) + " -> " +
                               m_stateMachine.stateToString(newState), "TcpClient");
    });
}

TcpClient::~TcpClient() {
    disconnectFromHost();
}

void TcpClient::connectToHost(const QString& host, quint16 port) {
    if (!m_stateMachine.canConnect()) {
        Logger::instance().warn("Cannot connect, current state: " + m_stateMachine.stateToString(m_stateMachine.state()), "TcpClient");
        return;
    }

    m_host = host;
    m_port = port;
    m_stateMachine.transitionToConnecting();
    m_socket->connectToHost(host, port);
}

void TcpClient::disconnectFromHost() {
    if (!m_stateMachine.canDisconnect()) {
        return;
    }

    m_stateMachine.transitionToDisconnecting();
    m_socket->disconnectFromHost();
    m_stateMachine.transitionToDisconnected();
}

void TcpClient::send(const QByteArray& data) {
    if (!m_stateMachine.canSend()) {
        Logger::instance().warn("Cannot send, current state: " + m_stateMachine.stateToString(m_stateMachine.state()), "TcpClient");
        return;
    }
    m_socket->write(data);
}

bool TcpClient::isConnected() const {
    return m_stateMachine.isConnected();
}

ConnectionState TcpClient::state() const {
    return m_stateMachine.state();
}

void TcpClient::setDataCallback(DataCallback callback) {
    m_dataCallback = callback;
}

void TcpClient::setConnectedCallback(ConnectedCallback callback) {
    m_connectedCallback = callback;
}

void TcpClient::setDisconnectedCallback(DisconnectedCallback callback) {
    m_disconnectedCallback = callback;
}

void TcpClient::setErrorCallback(ErrorCallback callback) {
    m_errorCallback = callback;
}

void TcpClient::setStateChangedCallback(StateChangedCallback callback) {
    m_stateMachine.setStateChangedCallback(callback);
}

void TcpClient::setReconnectPolicy(const ReconnectPolicy& policy) {
    m_reconnectPolicy = policy;
}

ReconnectPolicy TcpClient::reconnectPolicy() const {
    return m_reconnectPolicy;
}

void TcpClient::onConnected() {
    m_reconnectPolicy.resetRetry();
    m_stateMachine.transitionToConnected();
    if (m_connectedCallback) {
        m_connectedCallback();
    }
}

void TcpClient::onDisconnected() {
    bool wasConnected = m_stateMachine.isConnected() || m_stateMachine.isConnecting();
    m_stateMachine.transitionToDisconnected();
    if (m_disconnectedCallback) {
        m_disconnectedCallback();
    }
    if (wasConnected && m_reconnectPolicy.shouldReconnect()) {
        m_reconnectPolicy.incrementRetry();
        QTimer::singleShot(m_reconnectPolicy.nextRetryInterval().count(), this, &TcpClient::reconnect);
    }
}

void TcpClient::onReadyRead() {
    QByteArray data = m_socket->readAll();
    if (m_dataCallback) {
        m_dataCallback(data);
    }
}

void TcpClient::onError(QAbstractSocket::SocketError error) {
    Logger::instance().error("TCP error: " + QString::number(static_cast<int>(error)).toStdString(), "TcpClient");
    m_stateMachine.transitionToError();
    if (m_errorCallback) {
        m_errorCallback(error);
    }
}

void TcpClient::reconnect() {
    if (m_stateMachine.canConnect()) {
        m_stateMachine.transitionToConnecting();
        m_socket->connectToHost(m_host, m_port);
    }
}

} // namespace network
} // namespace sc