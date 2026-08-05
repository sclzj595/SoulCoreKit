#ifndef SOUL_CORE_LIFECYCLE_H
#define SOUL_CORE_LIFECYCLE_H

// ============================================================================
// lifecycle.h — ILifecycleManaged 生命周期管理接口 [v2.5.0]
// ============================================================================
//
// 对标 Spring 的 InitializingBean + DisposableBean。
// 服务、模块、控制器等需要生命周期管理的组件可实现此接口。
//
// 设计目的: 解耦 application 层与 cs 层的循环依赖。
//   ServiceRegistry 依赖 ILifecycleManaged（core 层），
//   而非 CsService（cs 层），打破 soul_application ↔ soul_cs。
//
// 实现者:
//   - sc::cs::CsService (-> initialize/shutdown)
//   - 未来: sc::Module, sc::CsController, 等
// ============================================================================

namespace sc {

/// @brief 生命周期管理接口
///
/// 对标 Spring 的 InitializingBean + DisposableBean。
/// 纯虚接口，不继承 QObject，支持与 QObject 多重继承。
///
/// @par 使用示例
/// @code
/// class MyService : public QObject, public ILifecycleManaged {
///     Q_OBJECT
/// public:
///     void initialize() override { ... }
///     void shutdown() override { ... }
/// };
/// @endcode
class ILifecycleManaged {
public:
    virtual ~ILifecycleManaged() = default;

    /// @brief 初始化（对标 @PostConstruct）
    virtual void initialize() = 0;

    /// @brief 销毁（对标 @PreDestroy）
    virtual void shutdown() = 0;
};

} // namespace sc

#endif // SOUL_CORE_LIFECYCLE_H