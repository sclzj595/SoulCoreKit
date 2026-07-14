#ifndef SOUL_NETWORK_INTERCEPTOR_I_INTERCEPTOR_H
#define SOUL_NETWORK_INTERCEPTOR_I_INTERCEPTOR_H

#include "soul/core/interface.h"
#include "soul/network/core/network_message.h"

namespace sc::network {

class IInterceptor : public IInterface {
public:
    ~IInterceptor() override = default;

    virtual void onRequest(NetworkMessage& request) = 0;
    virtual void onResponse(NetworkMessage& response) = 0;

    std::string interfaceName() const override {
        return "sc::network::IInterceptor";
    }
};

}

#endif
