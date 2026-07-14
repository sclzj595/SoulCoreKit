#include "soul/network/tcp/tcp_client_adapter.h"

namespace sc::network {

TcpClientAdapter::TcpClientAdapter(QObject* parent)
    : NetworkAdapterBase(parent), m_client(std::make_unique<TcpClient>()) {
    m_client->setConnectedCallback([this]() {
        onConnected();
    });
    m_client->setDisconnectedCallback([this]() {
        onDisconnected();
    });
    m_client->setDataCallback([this](const QByteArray& data) {
        onDataReceived(data);
    });
}

TcpClientAdapter::~TcpClientAdapter() = default;

void TcpClientAdapter::doConnect(const QUrl& url) {
    m_client->connectToHost(url.host(), url.port(80));
}

void TcpClientAdapter::doDisconnect() {
    m_client->disconnectFromHost();
}

bool TcpClientAdapter::doIsConnected() const {
    return m_client->isConnected();
}

Result<NetworkMessage> TcpClientAdapter::doSend(const NetworkMessage& message) {
    m_client->send(message.payload);
    updateState(NetworkState::Working);
    
    NetworkMessage response;
    response.metadata["sequence"] = ++m_sequence;
    response.metadata["packetType"] = "data";
    
    return response;
}

void TcpClientAdapter::doSendAsync(const NetworkMessage& message, ResponseCallback callback) {
    m_client->send(message.payload);
    updateState(NetworkState::Working);
    
    NetworkMessage response;
    response.metadata["sequence"] = ++m_sequence;
    response.metadata["packetType"] = "data";
    
    callback(response);
}

void TcpClientAdapter::onConnected() {
    updateState(NetworkState::Connected);
    emit connected();
}

void TcpClientAdapter::onDisconnected() {
    updateState(NetworkState::Disconnected);
    emit disconnected();
}

void TcpClientAdapter::onDataReceived(const QByteArray& data) {
    NetworkMessage msg;
    msg.payload = data;
    msg.metadata["packetType"] = "data";
    msg.metadata["sequence"] = ++m_sequence;
    emit received(msg);
}

}