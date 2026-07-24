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
#include <queue>
#include <string>
#include <unordered_map>
#include <QUrl>
#include <QTimer>
#include <QObject>
#include <QThread>
#include <cstdint>
#include "soul/network/core/inetwork.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT ConnectionPool : public QObject {
    Q_OBJECT
public:
    struct Config {
        int maxConnections = 10;
        int minConnections = 2;
        int idleTimeoutMs = 30000;
        int connectionTimeoutMs = 5000;
    };
    
    ConnectionPool(const Config& config);
    ~ConnectionPool();
    
    std::shared_ptr<INetwork> acquire(const QUrl& url);
    void release(std::shared_ptr<INetwork> connection);
    void closeAll();
    void cleanupIdleConnections();
    
    void setConfig(const Config& config);
    Config config() const;
    
private:
    struct ConnectionEntry {
        std::shared_ptr<INetwork> connection;
        bool inUse;
        uint64_t lastUsedTime;
    };
    
    std::shared_ptr<INetwork> createConnection(const QUrl& url);
    
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::queue<ConnectionEntry>> m_pools;
    Config m_config;
    QTimer m_cleanupTimer;
};

} // namespace network
} // namespace sc

#endif