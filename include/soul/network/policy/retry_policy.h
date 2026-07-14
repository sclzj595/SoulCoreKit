#ifndef SOUL_NETWORK_POLICY_RETRY_POLICY_H
#define SOUL_NETWORK_POLICY_RETRY_POLICY_H

#include <vector>
#include "soul/network/policy/inetwork_policy.h"

namespace sc::network {

enum class RetryStrategy {
    FixedInterval,
    ExponentialBackoff,
    LinearBackoff,
};

class RetryPolicy : public INetworkPolicy {
public:
    RetryPolicy();
    RetryPolicy(int maxRetries, RetryStrategy strategy = RetryStrategy::ExponentialBackoff);

    int maxRetries() const;
    RetryStrategy strategy() const;
    int nextDelay(int attempt) const;

    RetryPolicy& setMaxRetries(int max);
    RetryPolicy& setStrategy(RetryStrategy strategy);
    RetryPolicy& setBaseDelay(int ms);

    void apply(NetworkMessage& message) override;

    std::string interfaceName() const override {
        return "sc::network::RetryPolicy";
    }

private:
    int m_maxRetries = 3;
    RetryStrategy m_strategy = RetryStrategy::ExponentialBackoff;
    int m_baseDelay = 1000;
};

}

#endif