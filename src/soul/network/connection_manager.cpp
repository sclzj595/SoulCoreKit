// ============================================================================
// connection_manager.cpp — Client 端连接管理器实现
// ============================================================================

#include "soul/network/connection_manager.h"
#include "soul/network/policy/heartbeat_policy.h"

#include <QTimer>
#include <algorithm>
#include <cmath>

namespace sc {
namespace network {

// ============================================================================
// ConnectionManager 构造/析构
// ============================================================================

ConnectionManager::ConnectionManager(StateListener listener,
                                     QObject* parent)
    : QObject(parent)
    , m_stateListener(std::move(listener))
{
}

ConnectionManager::~ConnectionManager()
{
    disconnectAll();
}

// ============================================================================
// 连接注册
// ============================================================================

void ConnectionManager::registerConnection(const std::string& name,
                                           std::shared_ptr<INetwork> network,
                                           const ConnectionConfig& config)
{
    if (!network) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 如果已存在同名连接,先清理
    auto it = m_connections.find(name);
    if (it != m_connections.end()) {
        stopPolling(name);
        m_connections.erase(it);
    }

    ManagedConnection mc;
    mc.network = std::move(network);
    mc.config = config;
    mc.state = ManagedConnectionState::Disconnected;
    mc.retryCount = 0;
    m_connections[name] = std::move(mc);
}

void ConnectionManager::unregisterConnection(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    stopPolling(name);

    if (it->second.network) {
        it->second.network->disconnect();
    }

    m_connections.erase(it);
}

// ============================================================================
// 连接控制
// ============================================================================

void ConnectionManager::connectAll()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto& pair : m_connections) {
        const std::string& name = pair.first;
        auto& mc = pair.second;

        if (mc.state == ManagedConnectionState::Connected ||
            mc.state == ManagedConnectionState::Connecting) {
            continue;
        }

        if (mc.network) {
            mc.retryCount = 0;
            setState(name, ManagedConnectionState::Connecting);
            mc.network->connectTo(mc.config.url);
            startPolling(name);
        }
    }
}

void ConnectionManager::disconnectAll()
{
    // 先复制连接名称,避免在迭代中修改容器
    std::vector<std::string> names;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        names.reserve(m_connections.size());
        for (const auto& pair : m_connections) {
            names.push_back(pair.first);
        }
    }

    for (const auto& name : names) {
        disconnect(name);
    }
}

void ConnectionManager::connect(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    auto& mc = it->second;
    if (mc.state == ManagedConnectionState::Connected ||
        mc.state == ManagedConnectionState::Connecting) {
        return; // 已连接或正在连接
    }

    if (mc.network) {
        mc.retryCount = 0;
        setState(name, ManagedConnectionState::Connecting);
        mc.network->connectTo(mc.config.url);
        startPolling(name);
    }
}

void ConnectionManager::disconnect(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    auto& mc = it->second;
    stopPolling(name);

    if (mc.network) {
        mc.network->disconnect();
    }

    mc.retryCount = 0;
    setState(name, ManagedConnectionState::Disconnected);
}

// ============================================================================
// 状态查询
// ============================================================================

ManagedConnectionState ConnectionManager::state(const std::string& name) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return ManagedConnectionState::Disconnected;
    }

    return it->second.state;
}

bool ConnectionManager::isConnected(const std::string& name) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return false;
    }

    return it->second.state == ManagedConnectionState::Connected;
}

size_t ConnectionManager::activeConnectionCount() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return std::count_if(m_connections.begin(), m_connections.end(),
                         [](const auto& pair) {
                             return pair.second.state == ManagedConnectionState::Connected;
                         });
}

std::vector<std::string> ConnectionManager::connectionNames() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    std::vector<std::string> names;
    names.reserve(m_connections.size());
    for (const auto& pair : m_connections) {
        names.push_back(pair.first);
    }
    return names;
}

// ============================================================================
// 内部方法 — 状态轮询
// ============================================================================

void ConnectionManager::startPolling(const std::string& name)
{
    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    auto& mc = it->second;

    // 停止已有轮询
    if (mc.pollTimer) {
        mc.pollTimer->stop();
    } else {
        mc.pollTimer = std::make_unique<QTimer>();
    }

    // 每 500ms 检查一次连接状态
    mc.pollTimer->setInterval(500);
    QObject::connect(mc.pollTimer.get(), &QTimer::timeout,
                     [this, name]() { checkConnection(name); });
    mc.pollTimer->start();
}

void ConnectionManager::stopPolling(const std::string& name)
{
    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    auto& mc = it->second;
    if (mc.pollTimer) {
        mc.pollTimer->stop();
    }

    if (mc.heartbeat) {
        mc.heartbeat->stop();
        mc.heartbeat.reset();
    }
}

void ConnectionManager::checkConnection(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    auto& mc = it->second;
    if (!mc.network) {
        return;
    }

    bool connected = mc.network->isConnected();

    switch (mc.state) {
    case ManagedConnectionState::Connecting:
        if (connected) {
            setState(name, ManagedConnectionState::Connected);
            // 启用心跳(如果配置了)
            if (mc.config.enableHeartbeat && !mc.heartbeat) {
                mc.heartbeat = std::make_unique<HeartbeatPolicy>(
                    mc.config.heartbeatIntervalMs,
                    mc.config.heartbeatTimeoutMs);
                QObject::connect(mc.heartbeat.get(), &HeartbeatPolicy::heartbeatTimeout,
                                 [this, name]() { onHeartbeatTimeout(name); });
                mc.heartbeat->start(mc.network);
            }
        }
        break;

    case ManagedConnectionState::Connected:
        if (!connected) {
            setState(name, ManagedConnectionState::Disconnected);
            if (mc.config.autoReconnect) {
                scheduleReconnect(name);
            }
        }
        break;

    case ManagedConnectionState::Reconnecting:
        if (connected) {
            setState(name, ManagedConnectionState::Connected);
            // 重新启用心跳
            if (mc.config.enableHeartbeat && !mc.heartbeat) {
                mc.heartbeat = std::make_unique<HeartbeatPolicy>(
                    mc.config.heartbeatIntervalMs,
                    mc.config.heartbeatTimeoutMs);
                QObject::connect(mc.heartbeat.get(), &HeartbeatPolicy::heartbeatTimeout,
                                 [this, name]() { onHeartbeatTimeout(name); });
                mc.heartbeat->start(mc.network);
            }
        }
        break;

    default:
        break;
    }
}

// ============================================================================
// 内部方法 — 心跳
// ============================================================================

void ConnectionManager::onHeartbeatTimeout(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    auto& mc = it->second;

    // 停止心跳
    if (mc.heartbeat) {
        mc.heartbeat->stop();
        mc.heartbeat.reset();
    }

    // 断开并触发重连
    if (mc.network) {
        mc.network->disconnect();
    }

    setState(name, ManagedConnectionState::Disconnected);

    if (mc.config.autoReconnect) {
        scheduleReconnect(name);
    }
}

// ============================================================================
// 内部方法 — 重连
// ============================================================================

void ConnectionManager::scheduleReconnect(const std::string& name)
{
    // 注意: 此方法在持有锁时调用,需要小心
    // 使用 QTimer::singleShot 在事件循环中延迟执行

    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    auto& mc = it->second;

    // 检查是否超过最大重试次数
    if (mc.config.maxRetries > 0 && mc.retryCount >= mc.config.maxRetries) {
        setState(name, ManagedConnectionState::Error);
        return;
    }

    // [审计] setState 会 emit 信号并调用 m_stateListener, listener 回调中可能
    // unregisterConnection(name) 导致 m_connections 迭代器失效。因此先拷贝
    // retryCount/config, setState 后不再使用 it/mc 引用, 避免 UAF。
    auto retryCount = mc.retryCount;
    auto config = mc.config;
    setState(name, ManagedConnectionState::Reconnecting);

    auto interval = nextRetryInterval(retryCount, config);
    it = m_connections.find(name);
    if (it != m_connections.end()) {
        it->second.retryCount = retryCount + 1;
    }

    // 延迟执行重连
    QTimer::singleShot(interval, this, [this, name]() {
        tryReconnect(name);
    });
}

void ConnectionManager::tryReconnect(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    auto& mc = it->second;

    // 确认仍处于重连状态
    if (mc.state != ManagedConnectionState::Reconnecting) {
        return;
    }

    if (mc.network) {
        mc.network->connectTo(mc.config.url);
    }
}

// ============================================================================
// 内部方法 — 状态变更
// ============================================================================

void ConnectionManager::setState(const std::string& name,
                                 ManagedConnectionState newState,
                                 const std::string& message)
{
    auto it = m_connections.find(name);
    if (it == m_connections.end()) {
        return;
    }

    auto& mc = it->second;
    ManagedConnectionState oldState = mc.state;

    if (oldState == newState) {
        return;
    }

    mc.state = newState;

    // 发出 Qt 信号
    emit connectionStateChanged(name, newState, oldState);

    // [v2.5.1] 通过 StateListener 回调通知（替代 IEventBus，消除 soul_event 依赖）
    if (m_stateListener) {
        m_stateListener(name, newState, oldState, message);
    }
}

// ============================================================================
// 内部方法 — 指数退避
// ============================================================================

std::chrono::milliseconds ConnectionManager::nextRetryInterval(
    int retryCount, const ConnectionConfig& config)
{
    // 指数退避: baseInterval * 2^retryCount, 上限 maxInterval
    auto baseMs = config.baseInterval.count();
    auto maxMs = config.maxInterval.count();

    // 使用 double 避免整数溢出
    double factor = std::pow(2.0, static_cast<double>(retryCount));
    auto intervalMs = static_cast<long long>(static_cast<double>(baseMs) * factor);

    if (intervalMs > maxMs) {
        intervalMs = maxMs;
    }

    // 添加随机抖动 (±25%),避免惊群效应
    double jitter = 1.0 + (static_cast<double>(std::rand()) / RAND_MAX - 0.5) * 0.5;
    auto jitteredMs = static_cast<long long>(static_cast<double>(intervalMs) * jitter);

    if (jitteredMs < 100) {
        jitteredMs = 100; // 最小 100ms
    }

    return std::chrono::milliseconds(jitteredMs);
}

} // namespace network
} // namespace sc
