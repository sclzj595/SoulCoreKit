#ifndef SOUL_RPC_IRPC_TRANSPORT_H
#define SOUL_RPC_IRPC_TRANSPORT_H

// ============================================================================
// irpc_transport.h — RPC 传输接口 (v1.9.2: 迁移到 nlohmann/json)
// ============================================================================

#include <QString>
#include <functional>
#include <memory>

#include "soul/core/result.h"
#include "soul/utils/json/json_helper.h"

namespace sc { namespace rpc {

struct RpcRequest {
    QString serviceName;
    QString methodName;
    sc::json::Json params;
    QString requestId;
};

struct RpcResponse {
    bool success;
    sc::json::Json data;
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