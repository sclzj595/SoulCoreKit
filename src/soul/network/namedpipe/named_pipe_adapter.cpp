#include "soul/network/namedpipe/named_pipe_adapter.h"
#include "soul/core/result.h"
#include "soul/network/network_error.h"

namespace sc {
namespace network {

NamedPipeAdapter::NamedPipeAdapter() {
}

Result<NetworkMessage> NamedPipeAdapter::doSend(const NetworkMessage& message) {
    Q_UNUSED(message);
    return Result<NetworkMessage>(NetworkError(NetworkErrorCode::NotImplemented, "NamedPipe not implemented"));
}

void NamedPipeAdapter::doSendAsync(const NetworkMessage& message, ResponseCallback callback) {
    Q_UNUSED(message);
    callback(Result<NetworkMessage>(NetworkError(NetworkErrorCode::NotImplemented, "NamedPipe not implemented")));
}

void NamedPipeAdapter::doConnect(const QUrl& url) {
    Q_UNUSED(url);
    m_connected = false;
}

void NamedPipeAdapter::doDisconnect() {
    m_connected = false;
}

bool NamedPipeAdapter::doIsConnected() const {
    return m_connected;
}

} // namespace network
} // namespace sc