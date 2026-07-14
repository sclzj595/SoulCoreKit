#include "soul/network/pool/connection_pool.h"
#include "soul/network/factory/network_factory.h"
#include <chrono>

namespace sc::network {

ConnectionPool::ConnectionPool(const Config& config)
    : QObject(nullptr), m_config(config) {
    m_cleanupTimer.setInterval(m_config.idleTimeoutMs);
    connect(&m_cleanupTimer, &QTimer::timeout, [this]() {
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
    
    while (!pool.empty()) {
        ConnectionEntry entry = pool.front();
        pool.pop();
        
        if (entry.connection->isConnected()) {
            entry.inUse = true;
            entry.lastUsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            pool.push(entry);
            return entry.connection;
        }
    }
    
    return createConnection(url);
}

void ConnectionPool::release(std::shared_ptr<INetwork> connection) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& pair : m_pools) {
        auto& pool = pair.second;
        std::queue<ConnectionEntry> temp;
        
        while (!pool.empty()) {
            ConnectionEntry entry = pool.front();
            pool.pop();
            
            if (entry.connection == connection) {
                entry.inUse = false;
                entry.lastUsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
            }
            temp.push(entry);
        }
        
        while (!temp.empty()) {
            pool.push(temp.front());
            temp.pop();
        }
    }
}

void ConnectionPool::closeAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_pools) {
        auto& pool = pair.second;
        while (!pool.empty()) {
            auto entry = pool.front();
            pool.pop();
            entry.connection->disconnect();
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
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    for (auto& pair : m_pools) {
        auto& pool = pair.second;
        std::queue<ConnectionEntry> temp;
        
        while (!pool.empty()) {
            ConnectionEntry entry = pool.front();
            pool.pop();
            
            if (!entry.inUse && (now - entry.lastUsedTime) > m_config.idleTimeoutMs) {
                entry.connection->disconnect();
            } else {
                temp.push(entry);
            }
        }
        
        while (!temp.empty()) {
            pool.push(temp.front());
            temp.pop();
        }
    }
}

}