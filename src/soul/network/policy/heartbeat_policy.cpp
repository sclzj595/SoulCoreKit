#include "soul/network/policy/heartbeat_policy.h"
#include "soul/core/result.h"

namespace sc {
namespace network {

HeartbeatPolicy::HeartbeatPolicy(int intervalMs, int timeoutMs)
    : QObject(nullptr), m_interval(intervalMs), m_timeout(timeoutMs) {
    QObject::connect(&m_timer, &QTimer::timeout, [this]() {
        sendHeartbeat();
    });
    
    QObject::connect(&m_timeoutTimer, &QTimer::timeout, [this]() {
        emit heartbeatTimeout();
    });
}

HeartbeatPolicy::~HeartbeatPolicy() {
    stop();
}

void HeartbeatPolicy::apply(NetworkMessage& message) {
    message.metadata["heartbeatInterval"] = m_interval;
    message.metadata["heartbeatTimeout"] = m_timeout;
}

void HeartbeatPolicy::start(std::shared_ptr<INetwork> network) {
    m_network = network;
    m_timer.start(m_interval);
}

void HeartbeatPolicy::stop() {
    m_timer.stop();
    m_timeoutTimer.stop();
}

int HeartbeatPolicy::interval() const {
    return m_interval;
}

HeartbeatPolicy& HeartbeatPolicy::setInterval(int ms) {
    m_interval = ms;
    if (m_timer.isActive()) {
        m_timer.start(m_interval);
    }
    return *this;
}

int HeartbeatPolicy::timeout() const {
    return m_timeout;
}

HeartbeatPolicy& HeartbeatPolicy::setTimeout(int ms) {
    m_timeout = ms;
    return *this;
}

void HeartbeatPolicy::setHeartbeatMessage(const NetworkMessage& message) {
    m_heartbeatMessage = message;
}

NetworkMessage HeartbeatPolicy::heartbeatMessage() const {
    return m_heartbeatMessage;
}

void HeartbeatPolicy::sendHeartbeat() {
    auto network = m_network.lock();
    if (!network) return;
    
    m_timeoutTimer.start(m_timeout);
    
    if (!m_heartbeatMessage.api.isEmpty()) {
        network->sendAsync(m_heartbeatMessage, [this](const Result<NetworkMessage>&) {
            onResponse();
        });
    }
}

void HeartbeatPolicy::onResponse() {
    m_timeoutTimer.stop();
}

} // namespace network
} // namespace sc