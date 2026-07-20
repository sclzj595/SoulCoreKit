#include "soul/network/policy/reconnect_policy.h"

namespace sc {
namespace network {

ReconnectPolicy::ReconnectPolicy(bool enabled_, std::chrono::milliseconds interval_, int maxRetries_)
    : m_enabled(enabled_), m_baseInterval(interval_), m_maxRetries(maxRetries_) {}

bool ReconnectPolicy::enabled() const { return m_enabled; }
void ReconnectPolicy::setEnabled(bool enabled) { m_enabled = enabled; }

std::chrono::milliseconds ReconnectPolicy::baseInterval() const { return m_baseInterval; }
void ReconnectPolicy::setBaseInterval(std::chrono::milliseconds interval) { m_baseInterval = interval; }

int ReconnectPolicy::maxRetries() const { return m_maxRetries; }
void ReconnectPolicy::setMaxRetries(int maxRetries) { m_maxRetries = maxRetries; }

int ReconnectPolicy::currentRetry() const { return m_currentRetry; }

bool ReconnectPolicy::shouldReconnect() const {
    if (!m_enabled) return false;
    if (m_maxRetries <= 0) return true;
    return m_currentRetry < m_maxRetries;
}

void ReconnectPolicy::resetRetry() {
    m_currentRetry = 0;
}

void ReconnectPolicy::incrementRetry() {
    m_currentRetry++;
}
std::chrono::milliseconds ReconnectPolicy::nextRetryInterval() const {
    if (m_currentRetry <= 0) {
        return m_baseInterval;
    }
    std::chrono::milliseconds interval = m_baseInterval * (1LL << m_currentRetry);
    return std::min(interval, m_maxInterval);
}
void ReconnectPolicy::apply(NetworkMessage& message) {
    message.metadata["reconnectEnabled"] = m_enabled;
    message.metadata["reconnectInterval"] = static_cast<int>(m_baseInterval.count());
    message.metadata["reconnectMaxRetries"] = m_maxRetries;
}

} // namespace network
} // namespace sc
