/**
 * @file core/inetwork.h
 * @brief 网络接口抽象
 * @details 定义网络通信的统一接口，支持 HTTP/TCP/WebSocket 等协议
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_CORE_INETWORK_H
#define SOUL_NETWORK_CORE_INETWORK_H

#include <memory>
#include <functional>
#include <QUrl>
#ifndef Q_MOC_RUN
#include "soul/core/interface.h"
#include "soul/core/result.h"
#endif
#include "soul/network/core/network_message.h"
#include "soul/network/core/network_state.h"
#include "soul/network/policy/inetwork_policy.h"
#include "soul/network/interceptor/i_interceptor.h"

namespace sc {
namespace network {

class INetwork : public IInterface {
public:
    using ResponseCallback = std::function<void(const Result<NetworkMessage>&)>;

    ~INetwork() override = default;

    virtual void connectTo(const QUrl& url) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual Result<NetworkMessage> send(const NetworkMessage& message) = 0;
    virtual void sendAsync(const NetworkMessage& message, ResponseCallback callback) = 0;

    virtual NetworkState state() const = 0;

    virtual void setPolicy(std::shared_ptr<INetworkPolicy> policy) = 0;
    virtual void addInterceptor(std::shared_ptr<IInterceptor> interceptor) = 0;

    std::string interfaceName() const override {
        return "INetwork";
    }
};

} // namespace network
} // namespace sc

#endif
