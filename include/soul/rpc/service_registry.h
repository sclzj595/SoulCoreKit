#ifndef SOUL_RPC_SERVICE_REGISTRY_H
#define SOUL_RPC_SERVICE_REGISTRY_H

// ============================================================================
// service_registry.h — 服务注册发现核心接口 [v2.9.3 增强 / v3.0.0]
// ============================================================================
//
// v2.9.3 变更:
//   - ServiceInstance 新增: instanceId, metadata, status, version
//   - ServiceInstanceStatus 枚举 (Up/Down/Starting/Stopping/Unknown)
//   - IServiceRegistry 新增: getInstance(), getAllServices()
// v3.0.0: 移除 LoadBalancer (已迁移到 service_discovery.h 的 WeightedLoadBalancer)

#include <QString>
#include <QList>
#include <QHash>
#include <QMap>
#include <mutex>
#include <memory>
#include "soul/core/result.h"

namespace sc { namespace rpc {

// ============================================================================
// ServiceInstanceStatus — 实例状态 [v2.9.3 新增]
// ============================================================================

enum class ServiceInstanceStatus {
    Up,         // 健康，可接收请求
    Down,       // 不健康/不可达
    Starting,   // 正在启动
    Stopping,   // 正在停止
    Unknown     // 未知 (默认)
};

// ============================================================================
// ServiceInstance — 服务实例 [v2.9.3 增强]
// ============================================================================

struct ServiceInstance {
    QString serviceName;   // 服务名称 (如 "user-service")
    QString instanceId;    // 实例唯一 ID [v2.9.3 新增]
    QString host;
    int port = 0;
    qint64 timestamp = 0;

    // v2.9.3 新增
    ServiceInstanceStatus status = ServiceInstanceStatus::Unknown;
    QString version;                           // 服务版本 (如 "2.9.3")
    QMap<QString, QString> metadata;           // 扩展元数据 (zone/region/weight)

    /// @brief 生成唯一标识 (用于比较/去重)
    QString uniqueKey() const {
        if (!instanceId.isEmpty()) return instanceId;
        return QString("%1:%2:%3").arg(serviceName, host).arg(port);
    }
};

// ============================================================================
// IServiceRegistry — 服务注册接口
// ============================================================================

class IServiceRegistry {
public:
    virtual ~IServiceRegistry() = default;

    /// @brief 注册实例
    virtual Result<void> registerInstance(const ServiceInstance& instance) = 0;

    /// @brief 注销实例
    virtual Result<void> unregisterInstance(const QString& serviceName,
                                             const QString& host, int port) = 0;

    /// @brief 查询服务所有实例
    virtual Result<QList<ServiceInstance>> getInstances(const QString& serviceName) = 0;

    // v2.9.3 新增
    /// @brief 获取单个实例
    virtual Result<std::optional<ServiceInstance>>
        getInstance(const QString& serviceName, const QString& instanceId) = 0;

    /// @brief 获取所有已注册服务名称
    virtual Result<QStringList> getAllServices() = 0;
};

// ============================================================================
// InMemoryServiceRegistry — 内存实现
// ============================================================================

class InMemoryServiceRegistry : public IServiceRegistry {
public:
    Result<void> registerInstance(const ServiceInstance& instance) override;
    Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) override;
    Result<QList<ServiceInstance>> getInstances(const QString& serviceName) override;

    // v2.9.3 新增
    Result<std::optional<ServiceInstance>> getInstance(const QString& serviceName,
                                                        const QString& instanceId) override;
    Result<QStringList> getAllServices() override;

    /// @brief 按 instanceId 注销 [v2.9.3]
    Result<void> unregisterById(const QString& serviceName, const QString& instanceId);

private:
    mutable std::mutex m_mutex;
    QHash<QString, QList<ServiceInstance>> m_registry;
};

}} // namespace sc::rpc

#endif
