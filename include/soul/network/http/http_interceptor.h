#ifndef SOUL_NETWORK_HTTP_HTTP_INTERCEPTOR_H
#define SOUL_NETWORK_HTTP_HTTP_INTERCEPTOR_H

#include "soul/network/interceptor/i_interceptor.h"
#include "soul/network/http_request.h"
#include "soul/network/http_response.h"

namespace sc {
namespace network {

using HttpInterceptor = IInterceptor<HttpRequest, HttpResponse>;

} // namespace network
} // namespace sc

#endif