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
        if (driver) {
            auto result = driver->open(config);
            if (!result.isOk()) return nullptr;
        }
        return driver;
    }
private:
    DatabaseDriverFactory() = default;
    DatabaseDriverFactory(const DatabaseDriverFactory&) = delete;
    DatabaseDriverFactory& operator=(const DatabaseDriverFactory&) = delete;
};

} // namespace data
} // namespace sc

#endif