#ifndef SOUL_OBSERVABILITY_RESOURCE_POOL_MONITOR_H
#define SOUL_OBSERVABILITY_RESOURCE_POOL_MONITOR_H

// ============================================================================
// resource_pool_monitor.h — 资源池监控抽象
// ============================================================================
//
// 设计目标: 对标 SpringBoot Actuator 的 ResourcePoolIndicator,统一暴露
// ThreadPool / network::ConnectionPool / data::DbConnectionPool 等资源池的
// 水位指标(active/idle/max/utilization),集成到 observability::MetricsRegistry
// 作为 Gauge 指标导出,并提供阈值告警回调。
//
// 设计原则(遵循 project_memory 硬约束):
//   - 最小变更: 不修改现有资源池类的公共接口,通过 Adapter 模式包装
//   - 单一职责: 监控接口仅负责读取指标,不参与资源池生命周期管理
//   - 线程安全: 所有接口可并发调用
//   - RAII: 资源通过智能指针管理,严禁裸指针
//
// 用法:
//   auto& registry = sc::observability::ResourcePoolMonitorRegistry::instance();
//   registry.registerMonitor(std::make_shared<ThreadPoolMonitor>(ThreadPool::instance()));
//
//   // 定期采集并更新到 MetricsRegistry
//   sc::observability::ResourcePoolMetricsCollector collector(std::chrono::seconds(5));
//   collector.start();
//
//   // 阈值告警(利用率 > 80%)
//   registry.setAlertThreshold(0.8, [](const std::string& name, double util) {
//       SC_WARN("Resource pool {} utilization {:.1f}% exceeds threshold", name, util * 100);
//   });

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sc {

// 前置声明,避免头文件循环依赖(遵循 custom_user_instruction: 优先前置声明)
class ThreadPool;

namespace network { class ConnectionPool; }
namespace data     { class DbConnectionPool; }

namespace observability {

// ============================================================================
// ResourcePoolSnapshot — 资源池指标快照
// ============================================================================
struct ResourcePoolSnapshot {
    std::string name;           ///< 资源池名称(唯一标识)
    int          activeCount = 0; ///< 活跃资源数(正在使用)
    int          idleCount   = 0; ///< 空闲资源数(可复用)
    int          maxCount    = 0; ///< 最大资源数(容量上限)
    double       utilization = 0.0; ///< 利用率 = active/max, [0.0, 1.0]
};

// ============================================================================
// IResourcePoolMonitor — 资源池监控接口
// ============================================================================
//
// 所有资源池监控适配器的抽象基类。通过 Adapter 模式包装具体资源池,
// 避免修改现有类的公共接口(最小变更原则)。
//
// @thread_safety 所有方法可并发调用。
class IResourcePoolMonitor {
public:
    virtual ~IResourcePoolMonitor() = default;

    /// @return 资源池名称(唯一标识,用于注册表查找)
    virtual std::string name() const = 0;

    /// @return 活跃资源数(正在使用)
    virtual int activeCount() const = 0;

    /// @return 空闲资源数(可复用)
    virtual int idleCount() const = 0;

    /// @return 最大资源数(容量上限,0 表示无限制)
    virtual int maxCount() const = 0;

    /// @brief 获取当前快照(包含利用率计算)
    ResourcePoolSnapshot snapshot() const {
        ResourcePoolSnapshot s;
        s.name        = name();
        s.activeCount = activeCount();
        s.idleCount   = idleCount();
        s.maxCount    = maxCount();
        s.utilization = (s.maxCount > 0)
            ? static_cast<double>(s.activeCount) / static_cast<double>(s.maxCount)
            : 0.0;
        return s;
    }
};

// ============================================================================
// ResourcePoolMonitorRegistry — 资源池监控注册表(单例)
// ============================================================================
//
// 全局注册中心,统一管理所有 IResourcePoolMonitor 实例。
// 支持:
//   - 注册/注销监控适配器
//   - 批量获取快照
//   - 阈值告警回调(利用率超过阈值时触发)
//
// @thread_safety Thread-Safe
class ResourcePoolMonitorRegistry {
public:
    using AlertCallback = std::function<void(const std::string& name, double utilization)>;

    /// @brief 获取单例
    static ResourcePoolMonitorRegistry& instance();

    /// @brief 注册监控适配器(同名将覆盖)
    void registerMonitor(std::shared_ptr<IResourcePoolMonitor> monitor);

    /// @brief 注销指定名称的监控适配器
    void unregisterMonitor(const std::string& name);

    /// @brief 获取所有已注册监控的名称列表
    std::vector<std::string> names() const;

    /// @brief 获取所有资源池的当前快照
    std::vector<ResourcePoolSnapshot> snapshots() const;

    /// @brief 设置阈值告警(利用率超过 threshold 时触发 callback)
    /// @param threshold 阈值 [0.0, 1.0],默认 0.8
    /// @param callback  告警回调(为空则禁用告警)
    void setAlertThreshold(double threshold, AlertCallback callback);

    /// @brief 检查所有资源池水位,触发告警回调(由采集器定期调用)
    void checkAlerts() const;

    /// @brief 清空所有已注册的监控(仅用于测试)
    void clear();

private:
    ResourcePoolMonitorRegistry() = default;
    ResourcePoolMonitorRegistry(const ResourcePoolMonitorRegistry&) = delete;
    ResourcePoolMonitorRegistry& operator=(const ResourcePoolMonitorRegistry&) = delete;

    mutable std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<IResourcePoolMonitor>> m_monitors;
    double          m_alertThreshold = 0.8;
    AlertCallback   m_alertCallback;
};

// ============================================================================
// ThreadPoolMonitor — ThreadPool 适配器
// ============================================================================
//
// 包装 sc::ThreadPool,通过现有 activeThreadCount()/maxThreadCount() 接口
// 暴露水位指标。idle = max - active。
class ThreadPoolMonitor : public IResourcePoolMonitor {
public:
    /// @param pool  被监控的 ThreadPool 引用(调用方保证生命周期)
    /// @param name  监控名称(默认 "ThreadPool")
    explicit ThreadPoolMonitor(ThreadPool& pool, std::string name = "ThreadPool");

    std::string name() const override;
    int activeCount() const override;
    int idleCount() const override;
    int maxCount() const override;

private:
    ThreadPool&  m_pool;
    std::string  m_name;
};

// ============================================================================
// DbConnectionPoolMonitor — data::DbConnectionPool 适配器
// ============================================================================
//
// 包装 sc::data::DbConnectionPool,通过现有 getPoolSize()/getActiveConnections()
// 接口暴露水位指标。idle = total - active。
//
// 注意:DbConnectionPool 接口未暴露 maxSize,适配器通过构造时传入的 maxSize
// 记录容量上限(符合最小变更原则,不修改 DbConnectionPool 接口)。
class DbConnectionPoolMonitor : public IResourcePoolMonitor {
public:
    /// @param pool     被监控的 DbConnectionPool 引用
    /// @param maxSize  容量上限(用于利用率计算)
    /// @param name     监控名称(默认 "DbConnectionPool")
    explicit DbConnectionPoolMonitor(data::DbConnectionPool& pool,
                                      int maxSize,
                                      std::string name = "DbConnectionPool");

    std::string name() const override;
    int activeCount() const override;
    int idleCount() const override;
    int maxCount() const override;

private:
    data::DbConnectionPool& m_pool;
    int                     m_maxSize;
    std::string             m_name;
};

// ============================================================================
// NetworkConnectionPoolMonitor — network::ConnectionPool 适配器
// ============================================================================
//
// 包装 sc::network::ConnectionPool,通过新增的 activeCount()/idleCount()/
// maxCount() const 方法暴露水位指标。
class NetworkConnectionPoolMonitor : public IResourcePoolMonitor {
public:
    /// @param pool  被监控的 ConnectionPool 引用
    /// @param name  监控名称(默认 "NetworkConnectionPool")
    explicit NetworkConnectionPoolMonitor(network::ConnectionPool& pool,
                                           std::string name = "NetworkConnectionPool");

    std::string name() const override;
    int activeCount() const override;
    int idleCount() const override;
    int maxCount() const override;

private:
    network::ConnectionPool& m_pool;
    std::string              m_name;
};

// ============================================================================
// ResourcePoolMetricsCollector — 资源池指标定期采集器
// ============================================================================
//
// 后台线程定期采集所有已注册资源池的快照,更新到 MetricsRegistry 的 Gauge
// 指标,并触发阈值告警检查。
//
// 采集的指标(命名遵循 Prometheus 规范):
//   - resource_pool_active_count   {name="..."}  活跃数
//   - resource_pool_idle_count     {name="..."}  空闲数
//   - resource_pool_max_count      {name="..."}  容量上限
//   - resource_pool_utilization    {name="..."}  利用率 [0.0, 1.0]
//
// @thread_safety Thread-Safe,可多次 start/stop
class ResourcePoolMetricsCollector {
public:
    /// @param interval 采集间隔,默认 5 秒
    explicit ResourcePoolMetricsCollector(std::chrono::milliseconds interval = std::chrono::seconds(5));
    ~ResourcePoolMetricsCollector();

    ResourcePoolMetricsCollector(const ResourcePoolMetricsCollector&) = delete;
    ResourcePoolMetricsCollector& operator=(const ResourcePoolMetricsCollector&) = delete;

    /// @brief 启动后台采集线程(幂等,重复调用安全)
    void start();

    /// @brief 停止后台采集线程(幂等,重复调用安全)
    void stop();

    /// @return 是否正在运行
    bool isRunning() const noexcept;

    /// @brief 立即采集一次(同步,线程安全)
    void collectOnce();

private:
    void runLoop();

    std::chrono::milliseconds m_interval;
    std::atomic<bool>         m_running{false};
    std::atomic<bool>         m_stopFlag{false};
    std::thread               m_thread;
    std::mutex                m_threadMutex;  ///< 保护 m_thread 的 join/赋值(实现 @thread_safety Thread-Safe 契约)
};

} // namespace observability
} // namespace sc

#endif // SOUL_OBSERVABILITY_RESOURCE_POOL_MONITOR_H
