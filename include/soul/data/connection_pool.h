#ifndef SOUL_DATA_CONNECTION_POOL_H
#define SOUL_DATA_CONNECTION_POOL_H

#include <QString>
#include <QVariant>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include "soul/core/result.h"
#include "soul/data/database_driver.h"

namespace sc {
namespace data {

class DbConnectionPool {
public:
    virtual ~DbConnectionPool() = default;
    virtual Result<std::unique_ptr<IDatabaseDriver>> acquire() = 0;
    virtual void release(std::unique_ptr<IDatabaseDriver> driver) = 0;
    virtual int getPoolSize() const = 0;
    virtual int getActiveConnections() const = 0;
    virtual void closeAll() = 0;
};

class DefaultDbConnectionPool : public DbConnectionPool {
public:
    DefaultDbConnectionPool(const ConnectionConfig& config, int minSize = 2, int maxSize = 10) : m_config(config), m_minSize(minSize), m_maxSize(maxSize) {}

    Result<std::unique_ptr<IDatabaseDriver>> acquire() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_connections.empty()) {
            auto conn = std::move(m_connections.back());
            m_connections.pop_back();
            m_activeCount++;
            return Result<std::unique_ptr<IDatabaseDriver>>::ok(std::move(conn));
        }
        if (m_totalCount < m_maxSize) {
            auto driver = DatabaseDriverFactory::instance().create(m_config.type);
            if (driver) {
                auto result = driver->open(m_config);
                if (result.isOk()) {
                    m_totalCount++;
                    m_activeCount++;
                    return Result<std::unique_ptr<IDatabaseDriver>>::ok(std::move(driver));
                }
            }
            return Error(ErrorCode::DatabaseError, "Failed to create connection");
        }
        return Error(ErrorCode::ResourceExhausted, "Connection pool exhausted");
    }

    void release(std::unique_ptr<IDatabaseDriver> driver) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (driver && driver->isConnected()) {
            m_connections.push_back(std::move(driver));
        }
        m_activeCount--;
    }

    int getPoolSize() const override { return m_totalCount; }
    int getActiveConnections() const override { return m_activeCount; }
    void closeAll() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.clear();
        m_totalCount = 0;
        m_activeCount = 0;
    }

private:
    ConnectionConfig m_config;
    int m_minSize;
    int m_maxSize;
    int m_totalCount = 0;
    int m_activeCount = 0;
    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<IDatabaseDriver>> m_connections;
};

} // namespace data
} // namespace sc

#endif