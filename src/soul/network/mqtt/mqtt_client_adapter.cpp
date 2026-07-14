#include "soul/network/mqtt/mqtt_client_adapter.h"
#include "soul/core/result.h"
#include "soul/network/network_error.h"

namespace sc::network {

MqttClientAdapter::MqttClientAdapter() {
}

Result<NetworkMessage> MqttClientAdapter::doSend(const NetworkMessage& message) {
    Q_UNUSED(message);
    return Result<NetworkMessage>(NetworkError(NetworkErrorCode::NotImplemented, "MQTT not implemented"));
}

void MqttClientAdapter::doSendAsync(const NetworkMessage& message, ResponseCallback callback) {
    Q_UNUSED(message);
    callback(Result<NetworkMessage>(NetworkError(NetworkErrorCode::NotImplemented, "MQTT not implemented")));
}

void MqttClientAdapter::doConnect(const QUrl& url) {
    Q_UNUSED(url);
    m_connected = false;
}

void MqttClientAdapter::doDisconnect() {
    m_connected = false;
}

bool MqttClientAdapter::doIsConnected() const {
    return m_connected;
}

}