#include "soul/rpc/service_dispatcher.h"

namespace sc {
namespace rpc {

void ServiceDispatcher::registerService(const QString& serviceName, RpcHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_services.insert(serviceName, std::move(handler));
}

void ServiceDispatcher::unregisterService(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_services.remove(serviceName);
}

RpcResponse ServiceDispatcher::dispatch(const RpcRequest& request) {
    RpcHandler handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_services.find(request.serviceName);
        if (it == m_services.end()) {
            RpcResponse errorResp;
            errorResp.success = false;
            errorResp.errorMessage = QString("Service not found: %1").arg(request.serviceName);
            errorResp.requestId = request.requestId;
            return errorResp;
        }
        handler = it.value();
    }
    return handler(request);
}

QStringList ServiceDispatcher::registeredServices() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_services.keys();
}

} // namespace rpc
} // namespace sc