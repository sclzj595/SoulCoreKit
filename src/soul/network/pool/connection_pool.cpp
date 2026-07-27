#include "soul/network/pool/connection_pool.h"
#include "soul/network/factory/network_factory.h"
#include "soul/logging/log_macros.h"
#include <mutex>
#include <chrono>
#include <string>
#include <algorithm>

namespace sc {
namespace network {

// ---------------------------------------------------------------------------
// ConnectionGuard implementation
// ---------------------------------------------------------------------------

ConnectionPool::ConnectionGuard::ConnectionGuard(std::weak_ptr<ConnectionPool> pool, std::shared_ptr<INetwork> conn)
    : m_pool(std::move(pool)), m_conn(std::move(conn)) {}

ConnectionPool::ConnectionGuard::~ConnectionGuard() {
    if (m_conn) {
        auto pool = m_pool.lock();
        if (pool) {
            pool->release(m_conn);
        }
        // pool 已析构:连接随 m_conn 的 shared_ptr 析构自动释放,安全
    }
}

ConnectionPool::ConnectionGuard::ConnectionGuard(ConnectionGuard&& other) noexcept
    : m_pool(std::move(other.m_pool)), m_conn(std::move(other.m_conn)) {
    other.m_conn.reset();
}

ConnectionPool::ConnectionGuard& ConnectionPool::ConnectionGuard::operator=(ConnectionGuard&& other) noexcept {
    if (this != &other) {
        if (m_conn) {
            auto pool = m_pool.lock();
            if (pool) {
                pool->release(m_conn);
            }
        }
        m_pool = std::move(other.m_pool);
        m_conn = std::move(other.m_conn);
        other.m_conn.reset();
    }
    return *this;
}

std::shared_ptr<INetwork> ConnectionPool::ConnectionGuard::release() {
    m_pool.reset();
    return std::move(m_conn);
}

// ---------------------------------------------------------------------------
// ConnectionPool implementation
// ---------------------------------------------------------------------------

ConnectionPool::ConnectionPool(const Config& config)
    : QObject(nullptr), m_config(config) {
    m_cleanupTimer.setInterval(m_config.idleTimeoutMs);
    connect(&m_cleanupTimer, &QTimer::timeout, [this]() {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "cleanupIdleConnections", Qt::QueuedConnection);
            return;
        }
        cleanupIdleConnections();
    });
    m_cleanupTimer.start();
}

ConnectionPool::~ConnectionPool() {
    closeAll();
}

int ConnectionPool::countTotalLocked() const {
    int total = 0;
    for (const auto& pair : m_pools) {
        total += static_cast<int>(pair.second.size());
    }
    return total;
}

std::shared_ptr<INetwork> ConnectionPool::acquire(const QUrl& url) {
    std::unique_lock<std::mutex> lock(m_mutex);
    const std::string key = url.toString().toStdString();
    auto& pool = m_pools[key];

    // Helper: try to grab an available, connected, idle entry under lock.
    auto tryAcquireFromPool = [&]() -> std::shared_ptr<INetwork> {
        for (auto& entry : pool) {
            if (!entry.inUse && entry.connection && entry.connection->isConnected()) {
                entry.inUse = true;
                entry.lastUsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                return entry.connection;
            }
        }
        return nullptr;
    };

    if (auto conn = tryAcquireFromPool()) {
        return conn;
    }

    // If at capacity, wait for a release (up to connectionTimeoutMs).
    if (countTotalLocked() >= m_config.maxConnections) {
        if (m_cond.wait_for(lock,
                std::chrono::milliseconds(m_config.connectionTimeoutMs))
            == std::cv_status::timeout) {
            return nullptr;
        }
        // Woken up: retry once. If still no luck, fail (another thread may have grabbed it).
        // 注意:wait_for 期间 closeAll() 可能已执行 m_pools.clear() 使旧 pool 引用失效,
        // 此处必须重新查找 pool;若已被清除则返回 nullptr(与 V-5 同类修复)。
        auto poolIt = m_pools.find(key);
        if (poolIt == m_pools.end()) {
            return nullptr;
        }
        auto& refreshedPool = poolIt->second;
        for (auto& entry : refreshedPool) {
            if (!entry.inUse && entry.connection && entry.connection->isConnected()) {
                entry.inUse = true;
                entry.lastUsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                return entry.connection;
            }
        }
        return nullptr;
    }

    // Reserve a placeholder entry (inUse=true, connection=nullptr) so the total
    // count reflects the in-flight creation while we drop the lock to perform
    // the network IO outside the critical section.
    pool.push_back(ConnectionEntry{nullptr, true, 0});

    lock.unlock();
    std::shared_ptr<INetwork> network;
    try {
        network = NetworkFactory::instance().create(url);
        if (network) {
            network->connectTo(url);
        }
    } catch (const std::exception& e) {
        SC_ERROR("ConnectionPool: network creation failed: " + std::string(e.what()));
        network = nullptr;
    } catch (...) {
        // Blanket catch: pool-resize barrier. Network creation must not propagate
        // exceptions to the caller; failure is surfaced as a null connection.
        SC_ERROR("ConnectionPool: network creation failed: unknown exception");
        network = nullptr;
    }
    lock.lock();

    // 重新加锁后重新查找 pool 和 placeholder entryIt,
    // 因为 unlock 期间 closeAll() 可能已执行 m_pools.clear() 使旧引用/迭代器失效。
    auto poolIt = m_pools.find(key);
    if (poolIt == m_pools.end()) {
        // pool 已被 closeAll() 清除,直接返回新连接(不缓存)
        if (network) {
            return network;
        }
        m_cond.notify_one();
        return nullptr;
    }
    auto& refreshedPool = poolIt->second;
    // 查找 placeholder(inUse==true && connection==nullptr)
    auto entryIt = std::find_if(refreshedPool.begin(), refreshedPool.end(),
        [](const ConnectionEntry& e) { return e.inUse && !e.connection; });

    if (!network) {
        // Creation failed: remove placeholder and wake one waiter.
        if (entryIt != refreshedPool.end()) {
            refreshedPool.erase(entryIt);
        }
        m_cond.notify_one();
        return nullptr;
    }

    if (entryIt != refreshedPool.end()) {
        entryIt->connection = network;
        entryIt->lastUsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    } else {
        // placeholder 已被其他路径移除,将新连接作为新条目加入
        refreshedPool.push_back(ConnectionEntry{network, true,
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count())});
    }
    return network;
}

Result<ConnectionPool::ConnectionGuard> ConnectionPool::acquireGuarded(const QUrl& url) {
    auto conn = acquire(url);
    if (!conn) {
        return Error(ErrorCode::ResourceExhausted,
            "ConnectionPool::acquireGuarded: no connection available within timeout");
    }
    return ConnectionGuard(weak_from_this(), std::move(conn));
}

void ConnectionPool::release(std::shared_ptr<INetwork> connection) {
    if (!connection) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : m_pools) {
        for (auto& entry : pair.second) {
            if (entry.connection == connection) {
                entry.inUse = false;
                entry.lastUsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                m_cond.notify_one();
                return;
            }
        }
    }
}

void ConnectionPool::closeAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_pools) {
        auto& pool = pair.second;
        for (auto& entry : pool) {
            if (entry.connection) {
                entry.connection->disconnect();
            }
        }
    }
    m_pools.clear();
    m_cond.notify_all();
}

void ConnectionPool::setConfig(const Config& config) {
    // Hold the mutex only to update m_config; QTimer::setInterval must be invoked
    // on the timer's owning thread to respect Qt thread affinity and avoid TSan
    // reports on QTimer internal state.
    int newIdleTimeoutMs = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config = config;
        newIdleTimeoutMs = m_config.idleTimeoutMs;
    }
    QMetaObject::invokeMethod(&m_cleanupTimer,
        [this, newIdleTimeoutMs]() { m_cleanupTimer.setInterval(newIdleTimeoutMs); },
        Qt::QueuedConnection);
}

ConnectionPool::Config ConnectionPool::config() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void ConnectionPool::cleanupIdleConnections() {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());

    for (auto& pair : m_pools) {
        auto& pool = pair.second;
        auto it = pool.begin();
        while (it != pool.end()) {
            if (!it->inUse && (now - it->lastUsedTime) > static_cast<uint64_t>(m_config.idleTimeoutMs)) {
                if (it->connection) {
                    it->connection->disconnect();
                }
                it = pool.erase(it);
            } else {
                ++it;
            }
        }
    }
}

} // namespace network
} // namespace sc
