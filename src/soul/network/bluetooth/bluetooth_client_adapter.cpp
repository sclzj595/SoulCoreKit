#include "soul/network/bluetooth/bluetooth_client_adapter.h"
#include "soul/core/result.h"
#include "soul/network/network_error.h"

namespace sc {
namespace network {

BluetoothClientAdapter::BluetoothClientAdapter() {
}

Result<NetworkMessage> BluetoothClientAdapter::doSend(const NetworkMessage& message) {
    Q_UNUSED(message);
    return Result<NetworkMessage>(NetworkError(NetworkErrorCode::NotImplemented, "Bluetooth not implemented"));
}

void BluetoothClientAdapter::doSendAsync(const NetworkMessage& message, ResponseCallback callback) {
    Q_UNUSED(message);
    callback(Result<NetworkMessage>(NetworkError(NetworkErrorCode::NotImplemented, "Bluetooth not implemented")));
}

void BluetoothClientAdapter::doConnect(const QUrl& url) {
    Q_UNUSED(url);
    m_connected = false;
}

void BluetoothClientAdapter::doDisconnect() {
    m_connected = false;
}

bool BluetoothClientAdapter::doIsConnected() const {
    return m_connected;
}

} // namespace network
} // namespace sc