#ifndef SOUL_NETWORK_INTERCEPTOR_LOGGING_INTERCEPTOR_H
#define SOUL_NETWORK_INTERCEPTOR_LOGGING_INTERCEPTOR_H

#include "soul/network/interceptor/i_interceptor.h"

namespace sc::network {

class LoggingInterceptor : public IInterceptor {
public:
    ~LoggingInterceptor() override = default;

    void onRequest(NetworkMessage& request) override;
    void onResponse(NetworkMessage& response) override;

    std::string interfaceName() const override {
        return "sc::network::LoggingInterceptor";
    }
};

}

#endif