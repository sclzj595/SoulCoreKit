#ifndef SOUL_DATA_CONNECTION_POOL_H
#define SOUL_DATA_CONNECTION_POOL_H

#include <QString>
#include <QVariant>
#include <algorithm>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include "soul/core/result.h"
#include "soul/data/database_driver.h"

namespace sc {
namespace data {

// ============================================================================
// DbConnectionPool — 数据库连接池抽象接口
// ============================================================================
class DbConnectionPool {
public:
    virtual ~DbConnectionPool() = default;

    /// @brief 获取连接(阻塞直到可用或超时)
    /// @param timeoutMs 超时毫秒数(0 = 无限等待)
    virtual Result<std::unique_ptr<IDatabaseDriver>> acquire(int timeoutMs = 0) = 0;

    /// @brief 释放连接回池
    virtual void release(std::unique_ptr<IDatabaseDriver> driver) = 0;

    /// @return 池总连接数
    virtual int getPoolSize() const = 0;

    /// @return 活跃连接数
    virtual int getActiveConnections() const = 0;

    /// @return 空闲连接数
    virtual int getIdleConnections() const = 0;

    /// @brief 关闭所有连接
    virtual void closeAll() = 0;

    /// @brief 健康检查 — 清理断连并补充到 minSize
    virtual void healthCheck() = 0;
};

// ============================================================================
// DefaultDbConnectionPool — 默认数据库连接池实现 [v1.9.2 重构]
// ============================================================================
//
// 特性:
//   - 超时等待: acquire(timeoutMs) 使用条件变量等待空闲连接
//   - 动态扩缩: 按需创建连接(不超过 maxSize),空闲时回收
//   - 健康检查: release() 校验连接有效性,healthCheck() 清理断连
//   - 保底连接: 初始化时预创建 minSize 个连接
//
// @thread_safety Thread-Safe — 所有公共方法加锁保护
class DefaultDbConnectionPool : public DbConnectionPool {
public:
    /// @brief 使用 ConnectionConfig 构造
    DefaultDbConnectionPool(const ConnectionConfig& config, int minSize = 2, int maxSize = 10)
        : m_config(config), m_minSize(minSize), m_maxSize(maxSize),
          m_factory([this]() -> std::unique_ptr<IDatabaseDriver> {
              return DatabaseDriverFactory::instance().create(m_config.type);
          }) {}

    using DriverFactory = std::function<std::unique_ptr<IDatabaseDriver>()>;

    /// @brief 使用自定义工厂构造
    DefaultDbConnectionPool(DriverFactory factory, int minSize = 2, int maxSize = 10)
        : m_minSize(minSize), m_maxSize(maxSize), m_factory(std::move(factory)) {}

    // ========================================================================
    // acquire — 获取连接(支持超时等待) [v1.9.2 增强]
    // ========================================================================
    Result<std::unique_ptr<IDatabaseDriver>> acquire(int timeoutMs = 0) override {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto deadline = (timeoutMs > 0)
            ? std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs)
            : std::chrono::steady_clock::time_point::max();

        // 外层循环避免递归调用丢失超时预算
        for (;;) {
            // 1) 优先取空闲连接(含断连清理)
            while (!m_idleConnections.empty()) {
                auto conn = std::move(m_idleConnections.back());
                m_idleConnections.pop_back();
                if (!m_lastUsedTimes.empty()) m_lastUsedTimes.pop_back();
                if (conn && conn->isConnected()) {
                    m_activeCount++;
                    return Result<std::unique_ptr<IDatabaseDriver>>::ok(std::move(conn));
                }
                // 连接已断开,丢弃并减少计数
                if (m_totalCount > 0) m_totalCount--;
            }

            // 2) 有空位则创建新连接
            if (m_totalCount < m_maxSize) {
                auto driver = m_factory ? m_factory() : nullptr;
                if (driver) {
                    if (!driver->isConnected()) {
                        auto result = driver->open(m_config);
                        if (!result.isOk()) {
                            return Error(ErrorCode::DatabaseError, "Failed to create connection");
                        }
                    }
                    m_totalCount++;
                    m_activeCount++;
                    return Result<std::unique_ptr<IDatabaseDriver>>::ok(std::move(driver));
                }
                return Error(ErrorCode::DatabaseError, "Failed to create connection");
            }

            // 3) 池满 — 等待空闲连接或空位(保留剩余超时)
            if (deadline == std::chrono::steady_clock::time_point::max()) {
                m_condVar.wait(lock, [this]() {
                    return !m_idleConnections.empty() || m_totalCount < m_maxSize;
                });
            } else {
                auto now = std::chrono::steady_clock::now();
                if (now >= deadline) {
                    return Error(ErrorCode::Timeout, "Connection pool acquire timeout");
                }
                bool available = m_condVar.wait_until(lock, deadline, [this]() {
                    return !m_idleConnections.empty() || m_totalCount < m_maxSize;
                });
                if (!available) {
                    return Error(ErrorCode::Timeout, "Connection pool acquire timeout");
                }
            }
            // 唤醒后回到循环头重新尝试，不递归，超时预算得以保留
        }
    }

    // ========================================================================
    // release — 释放连接(含健康检查) [v1.9.2 增强]
    // ========================================================================
    void release(std::unique_ptr<IDatabaseDriver> driver) override {
        if (!driver) return;

        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_activeCount > 0) {
            m_activeCount--;
        }

        // 健康检查: 验证连接有效性
        if (driver->isConnected()) {
            m_idleConnections.push_back(std::move(driver));
            // 标记最后使用时间(用于空闲回收)
            m_lastUsedTimes.push_back(std::chrono::steady_clock::now());
        } else {
            // 连接已断开,丢弃
            if (m_totalCount > 0) m_totalCount--;
        }

        // 通知等待的 acquire() 调用
        m_condVar.notify_one();
    }

    int getPoolSize() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_totalCount;
    }
    int getActiveConnections() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_activeCount;
    }

    int getIdleConnections() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<int>(m_idleConnections.size());
    }

    void closeAll() override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_idleConnections.clear();
        m_lastUsedTimes.clear();
        m_totalCount = 0;
        m_activeCount = 0;
        m_condVar.notify_all();
    }

    // ========================================================================
    // healthCheck — 健康检查 [v1.9.2 新增]
    // ========================================================================
    //
    // 1. 清理断连的空闲连接
    // 2. 回收超时空闲连接(超过 idleTimeoutMs 未使用)
    // 3. 补充连接到 minSize
    void healthCheck() override {
        std::unique_lock<std::mutex> lock(m_mutex);

        // 1. 清理断连
        auto now = std::chrono::steady_clock::now();
        auto it = m_idleConnections.begin();
        auto timeIt = m_lastUsedTimes.begin();
        while (it != m_idleConnections.end()) {
            bool shouldRemove = false;
            if (!(*it) || !(*it)->isConnected()) {
                shouldRemove = true;
            } else if (m_idleTimeoutMs > 0 && timeIt != m_lastUsedTimes.end()) {
                auto idleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - *timeIt);
                if (idleDuration.count() > m_idleTimeoutMs && m_totalCount > m_minSize) {
                    shouldRemove = true;
                }
            }

            if (shouldRemove) {
                it = m_idleConnections.erase(it);
                timeIt = m_lastUsedTimes.erase(timeIt);
                if (m_totalCount > 0) m_totalCount--;
            } else {
                ++it;
                if (timeIt != m_lastUsedTimes.end()) ++timeIt;
            }
        }

        // 2. 补充到 minSize
        while (m_totalCount < m_minSize) {
            auto driver = m_factory ? m_factory() : nullptr;
            if (driver) {
                if (!driver->isConnected()) {
                    driver->open(m_config);
                }
                m_idleConnections.push_back(std::move(driver));
                m_lastUsedTimes.push_back(std::chrono::steady_clock::now());
                m_totalCount++;
            } else {
                break;
            }
        }
    }

    /// @brief 初始化连接池(预创建 minSize 个连接) [v1.9.2 新增]
    bool initialize() {
        std::unique_lock<std::mutex> lock(m_mutex);
        for (int i = m_totalCount; i < m_minSize; ++i) {
            auto driver = m_factory ? m_factory() : nullptr;
            if (!driver) return false;
            if (!driver->isConnected()) {
                auto result = driver->open(m_config);
                if (!result.isOk()) return false;
            }
            m_idleConnections.push_back(std::move(driver));
            m_lastUsedTimes.push_back(std::chrono::steady_clock::now());
            m_totalCount++;
        }
        return true;
    }

    /// @brief 设置空闲连接超时回收时间(ms,0=不回收) [v1.9.2 新增]
    void setIdleTimeout(int idleTimeoutMs) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_idleTimeoutMs = idleTimeoutMs;
    }

    /// @return 最小连接数
    int minSize() const { return m_minSize; }

    /// @return 最大连接数
    int maxSize() const { return m_maxSize; }

private:
    ConnectionConfig m_config;
    int m_minSize;
    int m_maxSize;
    int m_totalCount = 0;
    int m_activeCount = 0;
    int m_idleTimeoutMs = 300000;  ///< 默认 5 分钟空闲超时

    mutable std::mutex m_mutex;
    std::condition_variable m_condVar;  ///< [v1.9.2] 条件变量用于 acquire 等待
    std::vector<std::unique_ptr<IDatabaseDriver>> m_idleConnections;
    std::vector<std::chrono::steady_clock::time_point> m_lastUsedTimes;  ///< [v1.9.2] 连接最后使用时间
    DriverFactory m_factory;
};

} // namespace data
} // namespace sc

#endif