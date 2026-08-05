/**
 * @file pool/connection_pool.h
 * @brief 连接池类
 * @details 管理 HTTP 连接的复用和生命周期
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_POOL_CONNECTION_POOL_H
#define SOUL_NETWORK_POOL_CONNECTION_POOL_H

#include <memory>
#include <mutex>
#include <condition_variable>
#include <list>
#include <string>
#include <unordered_map>
#include <QUrl>
#include <QTimer>
#include <QObject>
#include <QThread>
#include <cstdint>
#include "soul/network/core/inetwork.h"
#include "soul/core/result.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT ConnectionPool : public QObject, public std::enable_shared_from_this<ConnectionPool> {
    Q_OBJECT
public:
    struct Config {
        int maxConnections = 10;
        int minConnections = 2;
        int idleTimeoutMs = 30000;
        int connectionTimeoutMs = 5000;
    };

    // RAII guard for acquired connections.
    // Automatically releases the connection back to the pool on destruction,
    // even if an exception is thrown between acquire and release.
    // Recommended usage: auto guard = pool.acquireGuarded(url).unwrap();
    //
    // 使用 std::weak_ptr<ConnectionPool> 防止 pool 先于 guard 析构时的 UAF
    class SC_NETWORK_EXPORT ConnectionGuard {
    public:
        ConnectionGuard() = default;
        ConnectionGuard(std::weak_ptr<ConnectionPool> pool, std::shared_ptr<INetwork> conn);
        ~ConnectionGuard();

        ConnectionGuard(const ConnectionGuard&) = delete;
        ConnectionGuard& operator=(const ConnectionGuard&) = delete;

        ConnectionGuard(ConnectionGuard&& other) noexcept;
        ConnectionGuard& operator=(ConnectionGuard&& other) noexcept;

        std::shared_ptr<INetwork> operator->() const { return m_conn; }
        std::shared_ptr<INetwork> connection() const { return m_conn; }
        INetwork& operator*() const { return *m_conn; }

        // Releases ownership without returning to pool.
        std::shared_ptr<INetwork> release();

        explicit operator bool() const { return m_conn != nullptr; }

    private:
        std::weak_ptr<ConnectionPool> m_pool;
        std::shared_ptr<INetwork> m_conn;
    };

    ConnectionPool(const Config& config);
    ~ConnectionPool();

    // Legacy API: caller must pair with release(). Risk of leak on exception.
    // Kept for backward compatibility; new code should prefer acquireGuarded().
    std::shared_ptr<INetwork> acquire(const QUrl& url);
    void release(std::shared_ptr<INetwork> connection);

    // RAII API: returns a guard that auto-releases on scope exit.
    // Waits up to connectionTimeoutMs for a connection to become available
    // when the pool is at capacity, returning ResourceExhausted error on timeout.
    Result<ConnectionGuard> acquireGuarded(const QUrl& url);

    void closeAll();
    void cleanupIdleConnections();

    void setConfig(const Config& config);
    Config config() const;

    /// @brief 设置动态扩缩容参数 [v1.9.4]
    /// @param minSize 最小连接数(空闲时维持的基准连接)
    /// @param maxSize 最大连接数(高峰时扩容上限)
    /// @details 基于负载自动调整:
    ///          - 扩容: 等待队列非空且当前连接数 < maxSize 时,按需创建新连接
    ///          - 缩容: cleanupIdleConnections() 清理超过 minSize 的空闲连接
    void setDynamicResize(int minSize, int maxSize) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.minConnections = minSize;
        m_config.maxConnections = maxSize;
    }

    /// @brief 查询是否启用动态扩缩容 [v1.9.4]
    /// @return true 当 minConnections > 0 且 maxConnections > minConnections
    bool isDynamicResizeEnabled() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_config.minConnections > 0 &&
               m_config.maxConnections > m_config.minConnections;
    }

    // v1.9.0: 资源池监控统计接口(供 NetworkConnectionPoolMonitor 适配器使用)
    // 不改变现有连接管理语义,仅暴露当前水位快照
    int activeCount() const;  ///< 正在使用的连接数
    int idleCount() const;    ///< 空闲可复用的连接数
    int maxCount() const;     ///< 容量上限(m_config.maxConnections)

private:
    struct ConnectionEntry {
        std::shared_ptr<INetwork> connection;
        bool inUse;
        uint64_t lastUsedTime;
    };

    int countTotalLocked() const;
    int countActiveLocked() const;  ///< 在锁内统计活跃连接数(供 activeCount() 复用)

    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
    std::unordered_map<std::string, std::list<ConnectionEntry>> m_pools;
    Config m_config;
    QTimer m_cleanupTimer;
};

} // namespace network
} // namespace sc

#endif