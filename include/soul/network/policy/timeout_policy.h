#ifndef SOUL_NETWORK_POLICY_TIMEOUT_POLICY_H
#define SOUL_NETWORK_POLICY_TIMEOUT_POLICY_H

#include "soul/network/policy/inetwork_policy.h"

namespace sc::network {

class TimeoutPolicy : public INetworkPolicy {
public:
    TimeoutPolicy(int timeoutMs = 30000);

    void apply(NetworkMessage& message) override;

    int timeout() const;
    TimeoutPolicy& setTimeout(int ms);

private:
    int m_timeout = 30000;
};

}

#endif
