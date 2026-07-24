/**
 * @file policy/timeout_policy.h
 * @brief 超时策略类
 * @details 管理网络操作的超时设置
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_POLICY_TIMEOUT_POLICY_H
#define SOUL_NETWORK_POLICY_TIMEOUT_POLICY_H

#include "soul/network/network_global.h"
#include "soul/network/policy/inetwork_policy.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT TimeoutPolicy : public INetworkPolicy {
public:
    TimeoutPolicy(int timeoutMs = 30000);

    void apply(NetworkMessage& message) override;

    int timeout() const;
    TimeoutPolicy& setTimeout(int ms);

    std::string interfaceName() const override {
        return "TimeoutPolicy";
    }

private:
    int m_timeout = 30000;
};

} // namespace network
} // namespace sc

#endif
