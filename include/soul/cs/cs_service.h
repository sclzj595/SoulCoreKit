#ifndef SOUL_CS_SERVICE_H
#define SOUL_CS_SERVICE_H

// ============================================================================
// cs_service.h — CS 服务层基类 [v2.5.0]
// ============================================================================
//
// 对标 SpringBoot 的 @Service 注解。
// 提供 DI 容器管理的服务生命周期基类。
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
/// 实现 ILifecycleManaged 接口，由 ServiceRegistry 统一管理 initialize/shutdown。
///
/// @par 使用示例
/// @code
/// class UserService : public CsService {
/// public:
///     UserService(std::shared_ptr<BaseRepository<User>> repo)
///         : CsService("UserService"), m_repo(std::move(repo)) {}
///
///     Result<std::vector<User>> findAll() { return m_repo->selectList(); }
///     Result<User> findById(int id) { return m_repo->selectById(id); }
///     Result<void> create(User& user) { return m_repo->insert(user); }
///
/// private:
///     std::shared_ptr<BaseRepository<User>> m_repo;
/// };
/// @endcode
class CsService : public QObject, public sc::ILifecycleManaged {
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

    /// @brief 服务初始化（对标 Spring 的 @PostConstruct）
    ///
    /// 在 DI 容器完成所有依赖注入后调用。
    /// 子类可重写此方法执行初始化逻辑。
    virtual void initialize() {}

    /// @brief 服务销毁（对标 Spring 的 @PreDestroy）
    ///
    /// 在 DI 容器销毁前调用。
    /// 子类可重写此方法执行清理逻辑。
    virtual void shutdown() {}

protected:
    QString m_serviceName;
    QString m_serviceVersion;
};

} // namespace sc::cs

#endif // SOUL_CS_SERVICE_H