#ifndef SOUL_NETWORK_POLICY_INETWORK_POLICY_H
#define SOUL_NETWORK_POLICY_INETWORK_POLICY_H

#include "soul/core/interface.h"
#include "soul/network/core/network_message.h"

namespace sc::network {

class INetworkPolicy : public IInterface {
public:
    ~INetworkPolicy() override = default;

    virtual void apply(NetworkMessage& message) = 0;

    std::string interfaceName() const override {
        return "sc::network::INetworkPolicy";
    }
};

}

#endif
