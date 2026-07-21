#include "soul/network/serial/serial_port_adapter.h"
#include "soul/core/result.h"
#include "soul/network/network_error.h"

namespace sc {
namespace network {

SerialPortAdapter::SerialPortAdapter() {
}

Result<NetworkMessage> SerialPortAdapter::doSend(const NetworkMessage& message) {
    Q_UNUSED(message);
    return Result<NetworkMessage>(NetworkError(NetworkErrorCode::NotImplemented, "SerialPort not implemented"));
}

void SerialPortAdapter::doSendAsync(const NetworkMessage& message, ResponseCallback callback) {
    Q_UNUSED(message);
    callback(Result<NetworkMessage>(NetworkError(NetworkErrorCode::NotImplemented, "SerialPort not implemented")));
}

void SerialPortAdapter::doConnect(const QUrl& url) {
    Q_UNUSED(url);
    m_connected = false;
}

void SerialPortAdapter::doDisconnect() {
    m_connected = false;
}

bool SerialPortAdapter::doIsConnected() const {
    return m_connected;
}

} // namespace network
} // namespace sc