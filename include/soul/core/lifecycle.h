#ifndef SOUL_CORE_LIFECYCLE_H
#define SOUL_CORE_LIFECYCLE_H

#include "soul/core/result.h"

// ============================================================================
// lifecycle.h — ILifecycle 统一生命周期接口 [v2.6.0]
// ============================================================================
//
// 语义模型:
//
//   Construct
//       ↓
//   initialize()    — 资源分配、DI 绑定、配置加载（可失败）
//       ↓
//   start()         — 启动服务、监听端口、开始消费（可失败）
//       ↓
//   Running         — 正常运行
//       ↓
//   stop()          — 停止接收新请求，等待进行中操作完成（保证执行）
//       ↓
//   shutdown()      — 释放所有资源（保证执行）
//       ↓
//   Destroy
//
// 设计原则:
//   1. initialize/start 返回 Result<void> — 允许失败并传播错误
//   2. stop/shutdown 返回 void — 保证执行，不因错误中断清理流程
//   3. stop 与 shutdown 语义分离:
//      - stop:   停止业务（关闭监听、拒绝新请求），但保留资源
//      - shutdown: 释放所有资源（关闭连接、销毁对象）
//   4. 保证调用顺序: initialize → start → stop → shutdown
//   5. shutdown 必须在 stop 之后调用，但允许跳过 stop 直接 shutdown
//
// 与旧接口的关系:
//   - ILifecycleManaged (v2.5.0): 两阶段 initialize/shutdown → 升级为 ILifecycle
//   - Module (v2.5.0): init/onStart/onStop/cleanup → 语义对齐
//   - BaseService (旧): bool start/void stop → 逐步迁移
//
// 实现者:
//   - sc::Module (兼容层)
//   - sc::cs::CsService
//   - sc::server::HttpServer
//   - sc::di::Container (Scope)
//   - 所有需要生命周期管理的核心组件
// ============================================================================

namespace sc {

/// @brief 生命周期状态枚举
enum class LifecycleState {
    Constructed,    ///< 构造完成，未初始化
    Initializing,   ///< initialize() 执行中
    Initialized,    ///< initialize() 成功完成
    Starting,       ///< start() 执行中
    Running,        ///< 正常运行
    Stopping,       ///< stop() 执行中
    Stopped,        ///< stop() 完成
    ShuttingDown,   ///< shutdown() 执行中
    Shutdown,       ///< 完全关闭
    Failed          ///< 初始化或启动失败
};

/// @brief 统一生命周期接口
///
/// 定义组件从初始化到销毁的完整生命周期语义。
/// 纯虚接口，不继承 QObject，支持与 QObject 多重继承。
///
/// @par 使用示例
/// @code
/// class MyService : public QObject, public ILifecycle {
///     Q_OBJECT
/// public:
///     Result<void> initialize() override {
///         // 加载配置、创建资源
///         return {};
///     }
///     Result<void> start() override {
///         // 启动监听、开始处理
///         return {};
///     }
///     void stop() noexcept override {
///         // 停止接收请求
///     }
///     void shutdown() noexcept override {
///         // 释放所有资源
///     }
/// };
/// @endcode
class ILifecycle {
public:
    virtual ~ILifecycle() = default;

    // ========================================================================
    // 生命周期方法
    // ========================================================================

    /// @brief 初始化 — 对标 @PostConstruct
    ///
    /// 职责: 资源分配、DI 绑定、配置加载、Schema 验证
    /// 调用时机: 构造之后、start 之前
    /// 失败语义: 返回 Error 将阻止组件启动，上层应执行逆序 cleanup
    /// 线程约束: 由调用方线程同步执行（通常为主线程）
    virtual Result<void> initialize() = 0;

    /// @brief 启动 — 对标 ContextRefreshedEvent
    ///
    /// 职责: 启动服务、监听端口、开始消费消息、启动定时器
    /// 调用时机: initialize 成功之后
    /// 失败语义: 返回 Error 将触发逆序 stop+shutdown
    /// 线程约束: 由调用方线程同步执行
    virtual Result<void> start() = 0;

    /// @brief 停止 — 对标 ContextClosedEvent
    ///
    /// 职责: 停止接收新请求、关闭监听端口、停止消费消息
    /// 调用时机: 应用关闭时，在 shutdown 之前
    /// 保证执行: 不返回错误，所有清理逻辑必须在此完成或推迟到 shutdown
    /// 线程约束: 由调用方线程同步执行
    /// 幂等性: 多次调用安全
    virtual void stop() noexcept = 0;

    /// @brief 销毁 — 对标 @PreDestroy
    ///
    /// 职责: 释放所有资源、关闭连接、销毁子对象
    /// 调用时机: stop 之后，析构之前
    /// 保证执行: 不返回错误，即使 stop 未调用也必须执行
    /// 线程约束: 由调用方线程同步执行
    /// 幂等性: 多次调用安全
    virtual void shutdown() noexcept = 0;

    // ========================================================================
    // 状态查询
    // ========================================================================

    /// @brief 获取当前生命周期状态
    virtual LifecycleState state() const = 0;

    /// @brief 是否处于运行状态
    bool isRunning() const { return state() == LifecycleState::Running; }

    /// @brief 是否已初始化（含 Running/Stopped）
    bool isInitialized() const {
        auto s = state();
        return s == LifecycleState::Initialized
            || s == LifecycleState::Starting
            || s == LifecycleState::Running
            || s == LifecycleState::Stopping
            || s == LifecycleState::Stopped;
    }
};

} // namespace sc

#endif // SOUL_CORE_LIFECYCLE_H
