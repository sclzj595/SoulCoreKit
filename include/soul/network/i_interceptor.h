#ifndef SOUL_NETWORK_I_INTERCEPTOR_H
#define SOUL_NETWORK_I_INTERCEPTOR_H

#include "soul/network/http_request.h"
#include "soul/network/http_response.h"
#include "soul/core/interface.h"

namespace sc {

class IInterceptor : public IInterface {
public:
    virtual ~IInterceptor() = default;

    virtual void onRequest(HttpRequest& request) = 0;
    virtual void onResponse(HttpResponse& response) = 0;

    std::string interfaceName() const override {
        return "sc::IInterceptor";
    }
};

}

#endif