#ifndef SOUL_NETWORK_HTTP_HTTP_INTERCEPTOR_H
#define SOUL_NETWORK_HTTP_HTTP_INTERCEPTOR_H

#include "soul/core/interface.h"
#include "soul/network/http_request.h"
#include "soul/network/http_response.h"

namespace sc::network {

class HttpInterceptor : public IInterface {
public:
    ~HttpInterceptor() override = default;

    virtual void onRequest(HttpRequest& request) = 0;
    virtual void onResponse(HttpResponse& response) = 0;

    std::string interfaceName() const override {
        return "sc::network::HttpInterceptor";
    }
};

}

#endif