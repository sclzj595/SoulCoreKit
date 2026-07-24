/**
 * @file policy/retry_policy.h
 * @brief 重试策略类
 * @details 实现重试逻辑，支持指数退避和最大重试次数
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_POLICY_RETRY_POLICY_H
#define SOUL_NETWORK_POLICY_RETRY_POLICY_H

#include <vector>
#include "soul/network/network_global.h"
#include "soul/network/policy/inetwork_policy.h"

namespace sc {
namespace network {

enum class RetryStrategy {
    FixedInterval,
    ExponentialBackoff,
    LinearBackoff,
};

class SC_NETWORK_EXPORT RetryPolicy : public INetworkPolicy {
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
        return "RetryPolicy";
    }

private:
    int m_maxRetries = 3;
    RetryStrategy m_strategy = RetryStrategy::ExponentialBackoff;
    int m_baseDelay = 1000;
};

} // namespace network
} // namespace sc

#endif