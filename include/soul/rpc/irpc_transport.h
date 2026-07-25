#ifndef SOUL_RPC_IRPC_TRANSPORT_H
#define SOUL_RPC_IRPC_TRANSPORT_H
#include <QString>
#include <QJsonObject>
#include <functional>
#include <memory>
#include "soul/core/result.h"
namespace sc { namespace rpc {
struct RpcRequest {
    QString serviceName;
    QString methodName;
    QJsonObject params;
    QString requestId;
};
struct RpcResponse {
    bool success;
    QJsonObject data;
    QString errorMessage;
    QString requestId;
};
class IRpcTransport {
public:
    virtual ~IRpcTransport() = default;
    virtual Result<RpcResponse> sendRequest(const RpcRequest& request) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
};
using RpcHandler = std::function<RpcResponse(const RpcRequest&)>;
}}
#endif