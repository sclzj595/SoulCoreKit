#ifndef SOUL_DATA_DATABASE_DRIVER_H
#define SOUL_DATA_DATABASE_DRIVER_H

#include <QString>
#include <QVariant>
#include <vector>
#include <map>
#include <memory>
#include "soul/core/result.h"

namespace sc {
namespace data {

enum class DatabaseType {
    SQLite,
    MySQL,
    PostgreSQL,
    MSSQL,
    Oracle
};

struct QueryResult {
    bool success = false;
    QString errorMessage;
    std::vector<std::map<QString, QVariant>> rows;
    int affectedRows = 0;
    QString lastInsertId;
};

struct ConnectionConfig {
    DatabaseType type;
    QString host;
    int port = 0;
    QString database;
    QString username;
    QString password;
    QString filePath;
    int connectionTimeoutMs = 30000;
};

class IDatabaseDriver {
public:
    virtual ~IDatabaseDriver() = default;
    virtual Result<void> open(const ConnectionConfig& config) = 0;
    virtual Result<void> close() = 0;
    virtual bool isConnected() const = 0;
    virtual Result<QueryResult> executeQuery(const QString& sql, const std::vector<QVariant>& params = {}) = 0;
    virtual Result<int> executeUpdate(const QString& sql, const std::vector<QVariant>& params = {}) = 0;
    virtual Result<void> beginTransaction() = 0;
    virtual Result<void> commit() = 0;
    virtual Result<void> rollback() = 0;
    virtual bool isInTransaction() const = 0;
    virtual QString getLastError() const = 0;
    virtual QString getConnectionId() const = 0;
    virtual DatabaseType getType() const = 0;

    // ========================================================================
    // v2.0.0 新增方法
    // ========================================================================

    /// @brief 执行迁移 SQL（用于数据库迁移系统）
    /// @param sql 迁移 SQL 语句
    /// @return 成功返回 Ok，失败返回错误
    ///
    /// @note 默认实现委托给 executeUpdate()，子类可覆写以添加额外逻辑
    virtual Result<void> executeMigrate(const QString& sql) {
        auto result = executeUpdate(sql, {});
        if (!result.isOk()) {
            return Error(ErrorCode::QueryFailed, getLastError());
        }
        return {};
    }

    /// @brief 获取最后一次 INSERT 操作的自增主键值 [v2.0.0 新增]
    /// @return 自增主键值（数据库无关）
    ///
    /// @note 各驱动实现数据库特定逻辑:
    ///   - SQLite: last_insert_rowid()
    ///   - MySQL: LAST_INSERT_ID()
    ///   - PostgreSQL: currval() 或 RETURNING 子句
    virtual Result<QVariant> lastInsertId() = 0;

    /// @brief 检查表是否存在
    /// @param tableName 表名
    /// @return true=表存在，false=表不存在
    ///
    /// @note 默认实现通过 SELECT 查询判断，子类可覆写以使用数据库特有方式
    virtual Result<bool> tableExists(const QString& tableName) {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }

        // 通用实现：尝试 SELECT 1 并捕获错误
        QString sql = QStringLiteral("SELECT 1 FROM %1 WHERE 1=0").arg(tableName);
        auto result = executeQuery(sql, {});
        return result.isOk();
    }
};

class DatabaseDriverFactory {
public:
    static DatabaseDriverFactory& instance() {
        static DatabaseDriverFactory inst;
        return inst;
    }
    std::unique_ptr<IDatabaseDriver> create(DatabaseType type);
    std::unique_ptr<IDatabaseDriver> create(const ConnectionConfig& config) {
        auto driver = create(config.type);
        if (!driver) {
            return nullptr;
        }
        auto result = driver->open(config);
        if (!result.isOk()) {
            m_lastError = QString::fromStdString(result.unwrapErr().toStdString());
            return nullptr;
        }
        return driver;
    }

    /// @brief 获取最后一次 create() 失败的错误信息
    [[nodiscard]] QString lastError() const { return m_lastError; }
private:
    DatabaseDriverFactory() = default;
    DatabaseDriverFactory(const DatabaseDriverFactory&) = delete;
    DatabaseDriverFactory& operator=(const DatabaseDriverFactory&) = delete;

    mutable QString m_lastError;
};

} // namespace data
} // namespace sc

#endif