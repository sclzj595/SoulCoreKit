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
#include "soul/network/core/inetwork.h"

namespace sc::network {

class ConnectionPool : public QObject {
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
    
    void setConfig(const Config& config);
    Config config() const;
    
private:
    struct ConnectionEntry {
        std::shared_ptr<INetwork> connection;
        bool inUse;
        uint64_t lastUsedTime;
    };
    
    std::shared_ptr<INetwork> createConnection(const QUrl& url);
    void cleanupIdleConnections();
    
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::queue<ConnectionEntry>> m_pools;
    Config m_config;
    QTimer m_cleanupTimer;
};

}

#endif