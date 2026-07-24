#include "soul/network/interceptor/logging_interceptor.h"
#include "soul/logging/log_macros.h"

namespace sc {
namespace network {

void LoggingInterceptor::onRequest(NetworkMessage& request) {
    QString api = request.api.isEmpty() ? "unknown" : request.api;
    int payloadSize = request.payload.size();
    SC_INFO_S("NETWORK", "REQUEST", QString("API: %1, PayloadSize: %2").arg(api).arg(payloadSize).toStdString());
}

void LoggingInterceptor::onResponse(NetworkMessage& response) {
    QString api = response.api.isEmpty() ? "unknown" : response.api;
    int statusCode = response.statusCode;
    int payloadSize = response.payload.size();
    qint64 duration = response.duration;
    SC_INFO_S("NETWORK", "RESPONSE", QString("API: %1, Status: %2, PayloadSize: %3, Duration: %4ms")
              .arg(api).arg(statusCode).arg(payloadSize).arg(duration).toStdString());
}

} // namespace network
} // namespace sc