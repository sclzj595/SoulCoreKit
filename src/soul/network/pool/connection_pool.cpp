#include "soul/network/pool/connection_pool.h"
#include "soul/network/factory/network_factory.h"
#include <mutex>
#include <chrono>
#include <string>

namespace sc {
namespace network {

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

std::shared_ptr<INetwork> ConnectionPool::acquire(const QUrl& url) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string key = url.toString().toStdString();
    auto& pool = m_pools[key];

    // Try to find an available (not in-use) connection
    for (auto& entry : pool) {
        if (!entry.inUse && entry.connection && entry.connection->isConnected()) {
            entry.inUse = true;
            entry.lastUsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            return entry.connection;
        }
    }

    // Count total connections across all pools
    int totalConnections = 0;
    for (const auto& pair : m_pools) {
        totalConnections += static_cast<int>(pair.second.size());
    }

    if (totalConnections >= m_config.maxConnections) {
        return nullptr;
    }

    // Create a new connection and add it to the pool
    auto network = NetworkFactory::instance().create(url);
    if (!network) {
        return nullptr;
    }
    network->connectTo(url);

    ConnectionEntry entry;
    entry.connection = network;
    entry.inUse = true;
    entry.lastUsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    pool.push_back(std::move(entry));

    return network;
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
}

void ConnectionPool::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_cleanupTimer.setInterval(m_config.idleTimeoutMs);
}

ConnectionPool::Config ConnectionPool::config() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

std::shared_ptr<INetwork> ConnectionPool::createConnection(const QUrl& url) {
    auto network = NetworkFactory::instance().create(url);
    network->connectTo(url);
    return network;
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