#ifndef SOUL_NETWORK_INTERCEPTOR_I_INTERCEPTOR_H
#define SOUL_NETWORK_INTERCEPTOR_I_INTERCEPTOR_H

#include "soul/core/interface.h"

namespace sc {
namespace network {

template<typename Request, typename Response>
class IInterceptor : public IInterface {
public:
    ~IInterceptor() override = default;

    virtual void onRequest(Request& request) = 0;
    virtual void onResponse(Response& response) = 0;

    std::string interfaceName() const override {
        return "IInterceptor";
    }
};

} // namespace network
} // namespace sc

#endif