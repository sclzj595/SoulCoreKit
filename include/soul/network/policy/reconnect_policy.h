/**
 * @file policy/reconnect_policy.h
 * @brief 重连策略类
 * @details 管理 WebSocket/TCP 连接断开后的自动重连逻辑
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_POLICY_RECONNECT_POLICY_H
#define SOUL_NETWORK_POLICY_RECONNECT_POLICY_H

#include <chrono>
#include "soul/network/network_global.h"
#include "soul/network/policy/inetwork_policy.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT ReconnectPolicy : public INetworkPolicy {
public:
    ReconnectPolicy() = default;
    ReconnectPolicy(bool enabled, std::chrono::milliseconds interval, int maxRetries);

    bool enabled() const;
    void setEnabled(bool enabled);

    std::chrono::milliseconds baseInterval() const;
    void setBaseInterval(std::chrono::milliseconds interval);

    std::chrono::milliseconds maxInterval() const;
    void setMaxInterval(std::chrono::milliseconds interval);

    int maxRetries() const;
    void setMaxRetries(int maxRetries);

    int currentRetry() const;

    bool shouldReconnect() const;
    void resetRetry();
    void incrementRetry();

    std::chrono::milliseconds nextRetryInterval() const;

    void apply(NetworkMessage& message) override;

    std::string interfaceName() const override {
        return "ReconnectPolicy";
    }

private:
    bool m_enabled = true;
    std::chrono::milliseconds m_baseInterval = std::chrono::seconds(1);
    std::chrono::milliseconds m_maxInterval = std::chrono::minutes(1);
    int m_maxRetries = 0;
    int m_currentRetry = 0;
};

} // namespace network
} // namespace sc

#endif