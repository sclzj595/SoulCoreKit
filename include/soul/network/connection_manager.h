#ifndef SOUL_NETWORK_CONNECTION_MANAGER_H
#define SOUL_NETWORK_CONNECTION_MANAGER_H

// ============================================================================
// connection_manager.h — Client 端连接管理器(断线重连/心跳保活/状态通知)
// ============================================================================
//
// 设计目标: 对标 SpringBoot 连接管理,统一管理 Client 端所有网络连接,
// 提供断线检测、指数退避重连、心跳保活、EventBus 状态通知。
//
// 核心组件:
//   - ConnectionManager:  连接管理器,统一管理多个 INetwork 连接
//   - ManagedConnectionState: 连接状态枚举(面向 UI 层)
//   - ConnectionConfig:    连接配置(重连/心跳参数)
//
// 设计原则:
//   - 单一职责: 仅负责连接生命周期管理,不处理业务逻辑
//   - 复用现有: ReconnectPolicy(指数退避) / HeartbeatPolicy(心跳)
//   - 最小变更: 在 soul_network 模块内新增,不修改现有接口
//   - 事件驱动: 通过 EventBus 通知 UI 层状态变更
//
// 用法:
//   auto mgr = std::make_shared<ConnectionManager>(eventBus);
//   mgr->registerConnection("main", httpClient, {.autoReconnect = true});
//   mgr->connectAll();
//
// 状态变迁:
//   Disconnected → Connecting → Connected
//   Connected → Disconnected → Reconnecting → Connecting → ...

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "soul/network/network_global.h"
#include "soul/network/core/inetwork.h"

namespace sc {
namespace network {

class HeartbeatPolicy;

// ============================================================================
// ManagedConnectionState — 连接状态枚举(面向 UI 层)
// ============================================================================
enum class ManagedConnectionState {
    Disconnected,  ///< 未连接
    Connecting,    ///< 正在连接(首次)
    Connected,     ///< 已连接
    Reconnecting,  ///< 正在重连(断线后自动重连)
    Error          ///< 连接错误(重连次数耗尽或配置错误)
};

/// @brief 状态转字符串
inline const char* toString(ManagedConnectionState s) {
    switch (s) {
    case ManagedConnectionState::Disconnected: return "Disconnected";
    case ManagedConnectionState::Connecting:   return "Connecting";
    case ManagedConnectionState::Connected:    return "Connected";
    case ManagedConnectionState::Reconnecting: return "Reconnecting";
    case ManagedConnectionState::Error:        return "Error";
    }
    return "Unknown";
}

// ============================================================================
// ConnectionConfig — 连接配置
// ============================================================================
struct ConnectionConfig {
    ConnectionConfig() = default;
    explicit ConnectionConfig(const QUrl& u) : url(u) {}

    std::string name;                    ///< 连接标识(唯一)
    QUrl        url;                     ///< 目标 URL
    bool        autoReconnect = true;    ///< 是否自动重连
    int         maxRetries = 0;          ///< 最大重试次数(0 = 无限)
    std::chrono::milliseconds baseInterval{1000};   ///< 初始重连间隔
    std::chrono::milliseconds maxInterval{60000};   ///< 最大重连间隔
    bool        enableHeartbeat = false;            ///< 是否启用心跳
    int         heartbeatIntervalMs = 30000;        ///< 心跳间隔(ms)
    int         heartbeatTimeoutMs = 10000;         ///< 心跳超时(ms)
};

// ============================================================================
// ConnectionManager — Client 端连接管理器
// ============================================================================
//
// 统一管理多个 INetwork 连接的生命周期:
//   1. 状态轮询 — QTimer 定期检查 isConnected(),检测断线
//   2. 指数退避重连 — 复用 ReconnectPolicy 的退避算法
//   3. 心跳保活 — 可选 HeartbeatPolicy 集成
//   4. 状态通知 — 通过 StateListener 回调或 Qt 信号通知
//
// 线程安全: 注册/查询加锁,状态轮询在 Qt 事件循环线程
//
// @thread_safety 注册/查询线程安全,回调在 Qt 事件循环线程
class SC_NETWORK_EXPORT ConnectionManager : public QObject {
    Q_OBJECT
public:
    /// @brief 状态变更监听器回调类型
    /// @param name          连接名称
    /// @param state         新状态
    /// @param previousState 旧状态
    /// @param message       附加消息（可选）
    using StateListener = std::function<void(const std::string& name,
                                             ManagedConnectionState state,
                                             ManagedConnectionState previousState,
                                             const std::string& message)>;

    /// @param listener 状态变更监听器(可选,为 nullptr 则不回调)
    /// @param parent   Qt 父对象
    explicit ConnectionManager(StateListener listener = nullptr,
                               QObject* parent = nullptr);
    ~ConnectionManager() override;

    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    // === 连接注册 ===

    /// @brief 注册连接(需在 connect() 之前调用)
    /// @param name    连接唯一标识
    /// @param network INetwork 实例
    /// @param config  连接配置
    void registerConnection(const std::string& name,
                            std::shared_ptr<INetwork> network,
                            const ConnectionConfig& config = {});

    /// @brief 注销连接
    void unregisterConnection(const std::string& name);

    // === 连接控制 ===

    /// @brief 连接所有已注册的连接
    void connectAll();

    /// @brief 断开所有连接
    void disconnectAll();

    /// @brief 连接指定名称的连接
    void connect(const std::string& name);

    /// @brief 断开指定名称的连接
    void disconnect(const std::string& name);

    // === 状态查询 ===

    /// @return 指定连接的状态
    ManagedConnectionState state(const std::string& name) const;

    /// @return 指定连接是否已连接
    bool isConnected(const std::string& name) const;

    /// @return 活跃连接数(Connected 状态)
    size_t activeConnectionCount() const;

    /// @return 所有已注册连接名称
    std::vector<std::string> connectionNames() const;

signals:
    /// @brief 连接状态变更信号
    void connectionStateChanged(const std::string& name,
                                ManagedConnectionState state,
                                ManagedConnectionState previousState);

private:
    struct ManagedConnection {
        std::shared_ptr<INetwork> network;
        ConnectionConfig config;
        ManagedConnectionState state = ManagedConnectionState::Disconnected;
        std::unique_ptr<QTimer> pollTimer;
        std::unique_ptr<HeartbeatPolicy> heartbeat;
        int retryCount = 0;
    };

    StateListener m_stateListener;  // [v2.5.1] 替代 IEventBus，消除 soul_event 依赖
    std::unordered_map<std::string, ManagedConnection> m_connections;
    // 递归锁: setState 在持锁期间会 emit 信号并调用 m_stateListener,
    // 外部回调可能重入加锁成员(如 isConnected/connect/disconnect),
    // 使用递归锁避免重入死锁。
    mutable std::recursive_mutex m_mutex;

    // === 内部方法 ===

    /// @brief 启动状态轮询(每 500ms 检查 isConnected())
    void startPolling(const std::string& name);

    /// @brief 停止状态轮询
    void stopPolling(const std::string& name);

    /// @brief 轮询检查连接状态
    void checkConnection(const std::string& name);

    /// @brief 心跳超时处理
    void onHeartbeatTimeout(const std::string& name);

    /// @brief 调度重连(使用指数退避)
    void scheduleReconnect(const std::string& name);

    /// @brief 执行重连
    void tryReconnect(const std::string& name);

    /// @brief 设置状态并通知
    void setState(const std::string& name, ManagedConnectionState newState,
                  const std::string& message = "");

    public:
    /// @brief 计算下一次重连间隔(指数退避,含随机抖动)
    /// @param retryCount 当前重试次数
    /// @param config 连接配置
    /// @return 退避间隔(最小 100ms)
    static std::chrono::milliseconds nextRetryInterval(int retryCount,
                                                       const ConnectionConfig& config);

private:
};

} // namespace network
} // namespace sc

#endif // SOUL_NETWORK_CONNECTION_MANAGER_H