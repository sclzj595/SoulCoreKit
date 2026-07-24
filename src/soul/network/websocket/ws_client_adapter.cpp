#include "soul/network/websocket/ws_client_adapter.h"

namespace sc {
namespace network {

WsClientAdapter::WsClientAdapter(QObject* parent)
    : NetworkAdapterBase(parent), m_client(std::make_unique<WebSocket>()) {
    m_client->setConnectedCallback([this]() {
        onConnected();
    });
    m_client->setDisconnectedCallback([this]() {
        onDisconnected();
    });
    m_client->setMessageCallback([this](const QString& message) {
        onTextMessageReceived(message);
    });
    m_client->setBinaryCallback([this](const QByteArray& data) {
        onBinaryMessageReceived(data);
    });
}

WsClientAdapter::~WsClientAdapter() = default;

void WsClientAdapter::doConnect(const QUrl& url) {
    m_client->connectToServer(url);
}

void WsClientAdapter::doDisconnect() {
    m_client->disconnectFromServer();
}

bool WsClientAdapter::doIsConnected() const {
    return m_client->isConnected();
}

Result<NetworkMessage> WsClientAdapter::doSend(const NetworkMessage& message) {
    if (message.metadata.value("messageType").toString() == "binary") {
        m_client->sendBinaryMessage(message.payload);
    } else {
        m_client->sendTextMessage(QString::fromUtf8(message.payload));
    }
    updateState(NetworkState::Working);
    
    NetworkMessage response;
    response.metadata["messageType"] = message.metadata.value("messageType", "text");
    
    return response;
}

void WsClientAdapter::doSendAsync(const NetworkMessage& message, ResponseCallback callback) {
    if (message.metadata.value("messageType").toString() == "binary") {
        m_client->sendBinaryMessage(message.payload);
    } else {
        m_client->sendTextMessage(QString::fromUtf8(message.payload));
    }
    updateState(NetworkState::Working);
    
    NetworkMessage response;
    response.metadata["messageType"] = message.metadata.value("messageType", "text");
    
    callback(response);
}

void WsClientAdapter::onConnected() {
    updateState(NetworkState::Connected);
    emit connected();
}

void WsClientAdapter::onDisconnected() {
    updateState(NetworkState::Disconnected);
    emit disconnected();
}

void WsClientAdapter::onTextMessageReceived(const QString& message) {
    NetworkMessage msg;
    msg.payload = message.toUtf8();
    msg.metadata["messageType"] = "text";
    emit received(msg);
}

void WsClientAdapter::onBinaryMessageReceived(const QByteArray& data) {
    NetworkMessage msg;
    msg.payload = data;
    msg.metadata["messageType"] = "binary";
    emit received(msg);
}

} // namespace network
} // namespace sc