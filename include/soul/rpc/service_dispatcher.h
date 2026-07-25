#ifndef SOUL_RPC_SERVICE_DISPATCHER_H
#define SOUL_RPC_SERVICE_DISPATCHER_H
#include <QString>
#include <QJsonObject>
#include <QHash>
#include <functional>
#include <memory>
#include <mutex>
#include "soul/core/result.h"
#include "soul/rpc/irpc_transport.h"
namespace sc { namespace rpc {
class ServiceDispatcher {
public:
    void registerService(const QString& serviceName, RpcHandler handler);
    void unregisterService(const QString& serviceName);
    RpcResponse dispatch(const RpcRequest& request);
    QStringList registeredServices() const;
private:
    mutable std::mutex m_mutex;
    QHash<QString, RpcHandler> m_services;
};
}}
#endif