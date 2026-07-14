#include "soul/network/policy/timeout_policy.h"

namespace sc::network {

TimeoutPolicy::TimeoutPolicy(int timeoutMs)
    : m_timeout(timeoutMs) {}

void TimeoutPolicy::apply(NetworkMessage& message) {
    message.metadata["timeout"] = m_timeout;
}

int TimeoutPolicy::timeout() const {
    return m_timeout;
}

TimeoutPolicy& TimeoutPolicy::setTimeout(int ms) {
    m_timeout = ms;
    return *this;
}

}
