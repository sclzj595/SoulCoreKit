#include "soul/network/policy/retry_policy.h"

namespace sc::network {

RetryPolicy::RetryPolicy() {}

RetryPolicy::RetryPolicy(int maxRetries, RetryStrategy strategy)
    : m_maxRetries(maxRetries), m_strategy(strategy) {}

int RetryPolicy::maxRetries() const { return m_maxRetries; }
RetryStrategy RetryPolicy::strategy() const { return m_strategy; }

int RetryPolicy::nextDelay(int attempt) const {
    switch (m_strategy) {
    case RetryStrategy::FixedInterval:
        return m_baseDelay;
    case RetryStrategy::ExponentialBackoff:
        return m_baseDelay * (1 << attempt);
    case RetryStrategy::LinearBackoff:
        return m_baseDelay * (attempt + 1);
    default:
        return m_baseDelay;
    }
}

RetryPolicy& RetryPolicy::setMaxRetries(int max) {
    m_maxRetries = max;
    return *this;
}

RetryPolicy& RetryPolicy::setStrategy(RetryStrategy strategy) {
    m_strategy = strategy;
    return *this;
}

RetryPolicy& RetryPolicy::setBaseDelay(int ms) {
    m_baseDelay = ms;
    return *this;
}

void RetryPolicy::apply(NetworkMessage& message) {
    message.metadata["maxRetries"] = m_maxRetries;
    message.metadata["retryStrategy"] = static_cast<int>(m_strategy);
    message.metadata["baseDelay"] = m_baseDelay;
}

}