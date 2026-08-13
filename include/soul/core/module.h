#ifndef SOUL_CORE_MODULE_H
#define SOUL_CORE_MODULE_H

#include <string>
#include <vector>
#include "soul/core/result.h"
#include "soul/core/lifecycle.h"

namespace sc {

// ============================================================================
// Module — 脚手架模块基类 [v2.6.0]
// ============================================================================
//
// 对标 SpringBoot 的 @Component / @Configuration 生命周期模型。
// 实现 ILifecycle 接口，与框架内所有生命周期管理组件语义统一。
//
// 生命周期 (对标 SpringBoot):
//   init()     → @PostConstruct      资源准备 (配置加载、DI 注册)
//   onStart()  → ContextRefreshed    服务启动 (网络监听、定时任务)
//   onStop()   → ContextClosed       服务停止 (停止接收新请求)
//   cleanup()  → @PreDestroy         资源释放 (关闭连接、释放内存)
//
// ILifecycle 映射 (v2.6.0 新增):
//   Module::init()     → ILifecycle::initialize()
//   Module::onStart()  → ILifecycle::start()
//   Module::onStop()   → ILifecycle::stop()
//   Module::cleanup()  → ILifecycle::shutdown()
//
// 依赖声明:
//   dependsOn() 返回依赖的模块名称列表，Scaffold 会拓扑排序后执行。
//
// 优先级:
//   priority() 返回优先级 (越大越先 init，越晚 cleanup)。默认 0。
//   同优先级按注册顺序。依赖关系优先于优先级。
//
// 条件装配 (对标 @ConditionalOnProperty):
//   isEnabled() 默认 true，子类可重写以根据配置决定是否装配。
class Module : public ILifecycle {
public:
    explicit Module(const std::string& name)
        : m_name(name), m_state(LifecycleState::Constructed) {}
    ~Module() override = default;

    // --- 基本信息 ---
    const std::string& name() const { return m_name; }

    // ========================================================================
    // ILifecycle 接口实现 (v2.6.0)
    // ========================================================================

    /// @brief ILifecycle::initialize() — 委托给 init()
    Result<void> initialize() final {
        m_state = LifecycleState::Initializing;
        auto result = init();
        if (result.isOk()) {
            m_state = LifecycleState::Initialized;
        } else {
            m_state = LifecycleState::Failed;
        }
        return result;
    }

    /// @brief ILifecycle::start() — 委托给 onStart()
    Result<void> start() final {
        m_state = LifecycleState::Starting;
        auto result = onStart();
        if (result.isOk()) {
            m_state = LifecycleState::Running;
        } else {
            m_state = LifecycleState::Failed;
        }
        return result;
    }

    /// @brief ILifecycle::stop() — 委托给 onStop()
    void stop() noexcept final {
        m_state = LifecycleState::Stopping;
        onStop();
        m_state = LifecycleState::Stopped;
    }

    /// @brief ILifecycle::shutdown() — 委托给 cleanup()
    void shutdown() noexcept final {
        m_state = LifecycleState::ShuttingDown;
        cleanup();
        m_state = LifecycleState::Shutdown;
    }

    /// @brief ILifecycle::state() — 当前生命周期状态
    LifecycleState state() const final { return m_state; }

    // ========================================================================
    // 生命周期钩子 (子类按需重写)
    // ========================================================================

    /// @brief 初始化阶段 — 资源准备、依赖注入注册
    ///
    /// 返回失败会触发回滚 (逆序 cleanup 已初始化的模块)。
    virtual Result<void> init() { return {}; }

    /// @brief 启动阶段 — 服务启动、网络监听、定时任务开启
    ///
    /// 返回失败会触发回滚 (逆序 stop + cleanup)。
    /// 默认实现空，兼容仅需要 init/cleanup 的简单模块。
    virtual Result<void> onStart() { return {}; }

    /// @brief 停止阶段 — 停止接收新请求、优雅停机
    ///
    /// 保证执行，不返回错误。默认空。
    virtual void onStop() {}

    /// @brief 清理阶段 — 资源释放、关闭连接
    ///
    /// 与 init() 配对：只要 init() 成功，无论 onStart() 是否执行，
    /// cleanup() 都会被调用。保证执行，不返回错误。默认空。
    virtual void cleanup() {}

    // --- 依赖声明与排序 ---

    /// @brief 声明依赖的模块名称列表。Scaffold 按拓扑序初始化。
    /// 默认无依赖。子类可重写以声明依赖。
    virtual std::vector<std::string> dependsOn() const { return {}; }

    /// @brief 优先级：越大越先 init、越晚 cleanup。默认 0。
    /// 注意：依赖关系优先于优先级。被依赖的模块总是先 init。
    virtual int priority() const { return 0; }

    // --- 条件装配 ---

    /// @brief 是否启用此模块。默认 true。
    /// 返回 false 时，Scaffold 跳过此模块 (不 init/start/stop/cleanup)。
    virtual bool isEnabled() const { return true; }

private:
    std::string m_name;
    LifecycleState m_state;
};

} // namespace sc

#endif // SOUL_CORE_MODULE_H
