#ifndef SOUL_NETWORK_CORE_NETWORK_ADAPTER_BASE_H
#define SOUL_NETWORK_CORE_NETWORK_ADAPTER_BASE_H

#include <memory>
#include <vector>
#include <functional>
#include <QUrl>
#include "soul/network/core/inetwork.h"
#include "soul/network/core/network_base.h"
#include "soul/network/core/network_state.h"
#include "soul/network/policy/inetwork_policy.h"
#include "soul/network/interceptor/i_interceptor.h"

namespace sc::network {

class NetworkAdapterBase : public NetworkBase, public INetwork {
    Q_OBJECT
public:
    explicit NetworkAdapterBase(QObject* parent = nullptr);
    ~NetworkAdapterBase() override = default;

    void connectTo(const QUrl& url) override;
    void disconnect() override;
    bool isConnected() const override;

    Result<NetworkMessage> send(const NetworkMessage& message) override;
    void sendAsync(const NetworkMessage& message, ResponseCallback callback) override;

    NetworkState state() const override;

    void setPolicy(std::shared_ptr<INetworkPolicy> policy) override;
    void addInterceptor(std::shared_ptr<IInterceptor> interceptor) override;

protected:
    void updateState(NetworkState newState);
    void applyPolicies(NetworkMessage& message);
    void applyRequestInterceptors(NetworkMessage& message);
    void applyResponseInterceptors(NetworkMessage& message);

    virtual Result<NetworkMessage> doSend(const NetworkMessage& message) = 0;
    virtual void doSendAsync(const NetworkMessage& message, ResponseCallback callback) = 0;
    virtual void doConnect(const QUrl& url) = 0;
    virtual void doDisconnect() = 0;
    virtual bool doIsConnected() const = 0;

private:
    NetworkState m_state = NetworkState::Created;
    std::shared_ptr<INetworkPolicy> m_policy;
    std::vector<std::shared_ptr<IInterceptor>> m_interceptors;
};

}

#endif