#ifndef SOUL_CS_SERVICE_H
#define SOUL_CS_SERVICE_H

// ============================================================================
// cs_service.h — CS 服务层基类 [v2.6.0]
// ============================================================================
//
// 对标 SpringBoot 的 @Service 注解。
// 提供 DI 容器管理的服务生命周期基类。
//
// v2.6.0 变更:
//   - 从 ILifecycleManaged 迁移到 ILifecycle
//   - initialize()/shutdown() 返回 Result<void>/void (对齐 ILifecycle)
//   - 新增 start()/stop() 钩子
//   - 新增 state() 状态查询
//
// 关系: CsService 由 DI Container 管理，被 CsController 注入使用。
// ============================================================================

#include <QObject>
#include <QString>

#include "soul/cs/cs_global.h"
#include "soul/core/lifecycle.h"

namespace sc::cs {

/// @brief CS 服务基类（对标 Spring 的 @Service）
///
/// 继承此类后，服务自动注册到 DI Container。
/// 生命周期由 DI 管理（对标 Spring 的 Singleton/Prototype Scope）。
/// 实现 ILifecycle 接口，由 ServiceRegistry 统一管理生命周期。
///
/// @par 使用示例
/// @code
/// class UserService : public CsService {
/// public:
///     UserService(std::shared_ptr<BaseRepository<User>> repo)
///         : CsService("UserService"), m_repo(std::move(repo)) {}
///
///     Result<void> initialize() override {
///         // 加载配置、预热缓存
///         return {};
///     }
///
///     Result<std::vector<User>> findAll() { return m_repo->selectList(); }
///     Result<User> findById(int id) { return m_repo->selectById(id); }
///
/// private:
///     std::shared_ptr<BaseRepository<User>> m_repo;
/// };
/// @endcode
class CsService : public QObject, public sc::ILifecycle {
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param serviceName 服务名称（用于日志和调试）
    /// @param parent 父对象
    explicit CsService(const QString& serviceName, QObject* parent = nullptr);

    ~CsService() override = default;

    /// @brief 获取服务名称
    QString serviceName() const { return m_serviceName; }

    /// @brief 获取服务版本
    QString serviceVersion() const { return m_serviceVersion; }

    /// @brief 设置服务版本
    void setServiceVersion(const QString& version) { m_serviceVersion = version; }

    // ========================================================================
    // ILifecycle 接口实现 (v2.6.0)
    // ========================================================================

    /// @brief 初始化 — 对标 Spring 的 @PostConstruct
    ///
    /// 在 DI 容器完成所有依赖注入后调用。
    /// 子类可重写此方法执行初始化逻辑（配置加载、资源分配）。
    /// @return 初始化结果，失败将阻止服务启动
    Result<void> initialize() override { return {}; }

    /// @brief 启动 — 对标 ContextRefreshedEvent
    ///
    /// 在 initialize 成功后调用。
    /// 子类可重写此方法启动服务（开始消费消息、启动定时任务）。
    /// @return 启动结果，失败将触发逆序 stop+shutdown
    Result<void> start() override { return {}; }

    /// @brief 停止 — 对标 ContextClosedEvent
    ///
    /// 停止接收新请求，等待进行中操作完成。
    /// 保证执行，不返回错误。默认空。
    void stop() noexcept override {}

    /// @brief 销毁 — 对标 @PreDestroy
    ///
    /// 在 DI 容器销毁前调用。
    /// 子类可重写此方法执行清理逻辑（关闭连接、释放资源）。
    /// 保证执行，不返回错误。默认空。
    void shutdown() noexcept override {}

    /// @brief 获取当前生命周期状态
    LifecycleState state() const override { return m_state; }

protected:
    QString m_serviceName;
    QString m_serviceVersion;
    LifecycleState m_state = LifecycleState::Constructed;
};

} // namespace sc::cs

#endif // SOUL_CS_SERVICE_H
