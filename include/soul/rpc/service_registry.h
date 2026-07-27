#ifndef SOUL_RPC_SERVICE_REGISTRY_H
#define SOUL_RPC_SERVICE_REGISTRY_H
#include <QString>
#include <QList>
#include <QHash>
#include <mutex>
#include <memory>
#include "soul/core/result.h"
namespace sc { namespace rpc {
struct ServiceInstance {
    QString serviceName;
    QString host;
    int port;
    qint64 timestamp;
};
class IServiceRegistry {
public:
    virtual ~IServiceRegistry() = default;
    virtual Result<void> registerInstance(const ServiceInstance& instance) = 0;
    virtual Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) = 0;
    virtual Result<QList<ServiceInstance>> getInstances(const QString& serviceName) = 0;
};
class InMemoryServiceRegistry : public IServiceRegistry {
public:
    Result<void> registerInstance(const ServiceInstance& instance) override;
    Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) override;
    Result<QList<ServiceInstance>> getInstances(const QString& serviceName) override;
private:
    mutable std::mutex m_mutex;
    QHash<QString, QList<ServiceInstance>> m_registry;
};
class LoadBalancer {
public:
    ServiceInstance select(const QList<ServiceInstance>& instances);
    void setRoundRobin();
    void setRandom();
private:
    bool m_roundRobin = true;
    int m_counter = 0;
    std::mutex m_mutex;
};
}}
#endif