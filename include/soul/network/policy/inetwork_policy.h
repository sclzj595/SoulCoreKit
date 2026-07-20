/**
 * @file policy/inetwork_policy.h
 * @brief 网络策略接口
 * @details 策略模式接口，定义重试、超时、重连等策略的统一接口
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_POLICY_INETWORK_POLICY_H
#define SOUL_NETWORK_POLICY_INETWORK_POLICY_H

#ifndef Q_MOC_RUN
#include "soul/core/interface.h"
#endif
#include "soul/network/core/network_message.h"

namespace sc {
namespace network {

class INetworkPolicy : public IInterface {
public:
    ~INetworkPolicy() override = default;

    virtual void apply(NetworkMessage& message) = 0;

    std::string interfaceName() const override {
        return "INetworkPolicy";
    }
};

} // namespace network
} // namespace sc

#endif
