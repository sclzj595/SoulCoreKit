#include "soul/network/core/network_adapter_base.h"

namespace sc {
namespace network {

NetworkAdapterBase::NetworkAdapterBase(QObject* parent)
    : NetworkBase(parent) {}

void NetworkAdapterBase::connectTo(const QUrl& url) {
    updateState(NetworkState::Connecting);
    doConnect(url);
}

void NetworkAdapterBase::disconnect() {
    doDisconnect();
    updateState(NetworkState::Disconnected);
    emit disconnected();
}

bool NetworkAdapterBase::isConnected() const {
    return doIsConnected();
}

Result<NetworkMessage> NetworkAdapterBase::send(const NetworkMessage& message) {
    NetworkMessage requestMsg = message;
    
    applyPolicies(requestMsg);
    applyRequestInterceptors(requestMsg);
    
    auto result = doSend(requestMsg);
    
    if (result.isOk()) {
        applyResponseInterceptors(result.unwrap());
    }
    
    return result;
}

void NetworkAdapterBase::sendAsync(const NetworkMessage& message, ResponseCallback callback) {
    NetworkMessage requestMsg = message;
    
    applyPolicies(requestMsg);
    applyRequestInterceptors(requestMsg);
    
    doSendAsync(requestMsg, [this, callback](const Result<NetworkMessage>& result) {
        if (result.isOk()) {
            NetworkMessage responseMsg = result.unwrap();
            applyResponseInterceptors(responseMsg);
            callback(responseMsg);
        } else {
            callback(result);
        }
    });
}

NetworkState NetworkAdapterBase::state() const {
    return m_state;
}

void NetworkAdapterBase::setPolicy(std::shared_ptr<INetworkPolicy> policy) {
    m_policy = policy;
}

void NetworkAdapterBase::addInterceptor(std::shared_ptr<IInterceptor<NetworkMessage, NetworkMessage>> interceptor) {
    if (interceptor) {
        m_interceptors.push_back(interceptor);
    }
}

void NetworkAdapterBase::updateState(NetworkState newState) {
    m_state = newState;
}

void NetworkAdapterBase::applyPolicies(NetworkMessage& message) {
    if (m_policy) {
        m_policy->apply(message);
    }
}

void NetworkAdapterBase::applyRequestInterceptors(NetworkMessage& message) {
    for (const auto& interceptor : m_interceptors) {
        interceptor->onRequest(message);
    }
}

void NetworkAdapterBase::applyResponseInterceptors(NetworkMessage& message) {
    for (auto it = m_interceptors.rbegin(); it != m_interceptors.rend(); ++it) {
        (*it)->onResponse(message);
    }
}

} // namespace network
} // namespace sc