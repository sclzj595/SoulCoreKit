#ifndef SOUL_NETWORK_CORE_INETWORK_H
#define SOUL_NETWORK_CORE_INETWORK_H

#include <memory>
#include <functional>
#include <QUrl>
#include "soul/core/interface.h"
#include "soul/core/result.h"
#include "soul/network/core/network_message.h"
#include "soul/network/core/network_state.h"
#include "soul/network/policy/inetwork_policy.h"
#include "soul/network/interceptor/i_interceptor.h"

namespace sc::network {

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
        return "sc::network::INetwork";
    }
};

}

#endif
