#include "soul/network/tcp_client.h"
#include <QTimer>
#include "soul/logging/logger.h"

namespace sc {

TcpClient::TcpClient(QObject* parent) : QObject(parent) {
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpClient::onError);
}

TcpClient::~TcpClient() {}

void TcpClient::connectToHost(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;
    m_socket->connectToHost(host, port);
}

void TcpClient::disconnectFromHost() {
    m_socket->disconnectFromHost();
}

void TcpClient::send(const QByteArray& data) {
    if (isConnected()) {
        m_socket->write(data);
    }
}

bool TcpClient::isConnected() const {
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
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

void TcpClient::setReconnectPolicy(const network::ReconnectPolicy& policy) {
    m_reconnectPolicy = policy;
}

network::ReconnectPolicy TcpClient::reconnectPolicy() const {
    return m_reconnectPolicy;
}

void TcpClient::onConnected() {
    m_reconnectPolicy.resetRetry();
    if (m_connectedCallback) {
        m_connectedCallback();
    }
}

void TcpClient::onDisconnected() {
    if (m_disconnectedCallback) {
        m_disconnectedCallback();
    }
    if (m_reconnectPolicy.shouldReconnect()) {
        m_reconnectPolicy.incrementRetry();
        QTimer::singleShot(m_reconnectPolicy.interval.count(), this, &TcpClient::reconnect);
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
    if (m_errorCallback) {
        m_errorCallback(error);
    }
}

void TcpClient::reconnect() {
    if (!isConnected()) {
        m_socket->connectToHost(m_host, m_port);
    }
}

}