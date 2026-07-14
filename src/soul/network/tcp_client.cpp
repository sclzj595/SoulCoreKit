#include "soul/network/tcp_client.h"

namespace sc {

TcpClient::TcpClient(QObject* parent) : QObject(parent) {
    m_socket = new QTcpSocket(this);
}

TcpClient::~TcpClient() {}

void TcpClient::connectToHost(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;
    m_socket->connectToHost(host, port);

    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpClient::onError);
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

void TcpClient::setAutoReconnect(bool enabled) {
    m_autoReconnect = enabled;
}

bool TcpClient::autoReconnect() const {
    return m_autoReconnect;
}

void TcpClient::onConnected() {
    if (m_connectedCallback) {
        m_connectedCallback();
    }
}

void TcpClient::onDisconnected() {
    if (m_disconnectedCallback) {
        m_disconnectedCallback();
    }
}

void TcpClient::onReadyRead() {
    QByteArray data = m_socket->readAll();
    if (m_dataCallback) {
        m_dataCallback(data);
    }
}

void TcpClient::onError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error);
}

}