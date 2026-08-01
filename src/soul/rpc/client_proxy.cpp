#include "soul/rpc/client_proxy.h"
#include <QUuid>

namespace sc {
namespace rpc {

ClientProxy::ClientProxy(std::shared_ptr<IRpcTransport> transport,
                         std::shared_ptr<ISerializer> serializer)
    : m_transport(std::move(transport))
    , m_serializer(std::move(serializer))
{
}

Result<sc::json::Json> ClientProxy::call(const QString& service, const QString& method,
                                          const sc::json::Json& params) {
    RpcRequest request;
    request.serviceName = service;
    request.methodName = method;
    request.params = params;
    request.requestId = generateRequestId();

    auto result = m_transport->sendRequest(request);
    if (result.isErr()) {
        return result.unwrapErr();
    }

    auto response = result.unwrap();
    if (!response.success) {
        return Result<sc::json::Json>(Error(ErrorCode::InternalError, response.errorMessage));
    }

    return Result<sc::json::Json>(response.data);
}

QString ClientProxy::generateRequestId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace rpc
} // namespace sc