#include "soul/data/database_driver.h"
#include "soul/data/database_driver_base.h"
#include "soul/data/mysql_driver.h"
#include "soul/data/postgres_driver.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QUuid>

namespace sc {
namespace data {

class BaseSqlDriver : public IDatabaseDriver {
public:
    ~BaseSqlDriver() override { (void)close(); }

    Result<void> close() override {
        if (m_db.isOpen()) {
            QString dbName = m_db.connectionName();
            m_db.close();
            // v3.0.0: 释放 m_db 对连接的引用后再 removeDatabase,
            // 否则 Qt 会警告 "connection is still in use" (导致 QTest 退出码非 0)。
            m_db = QSqlDatabase();
            QSqlDatabase::removeDatabase(dbName);
        }
        return {};
    }

    bool isConnected() const override { return m_db.isOpen(); }

    Result<QueryResult> executeQuery(const QString& sql, const std::vector<QVariant>& params) override {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }

        QSqlQuery query(m_db);
        if (!query.prepare(sql)) {
            return Error(ErrorCode::QueryFailed, query.lastError().text());
        }

        for (size_t i = 0; i < params.size(); ++i) {
            query.bindValue(static_cast<int>(i), params[i]);
        }

        if (!query.exec()) {
            return Error(ErrorCode::QueryFailed, query.lastError().text());
        }

        QueryResult result;
        result.success = true;

        QSqlRecord record = query.record();
        while (query.next()) {
            std::map<QString, QVariant> row;
            for (int i = 0, cnt = static_cast<int>(record.count()); i < cnt; ++i) {
                row[record.fieldName(i)] = query.value(i);
            }
            result.rows.push_back(row);
        }

        return result;
    }

    Result<int> executeUpdate(const QString& sql, const std::vector<QVariant>& params) override {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }

        QSqlQuery query(m_db);
        if (!query.prepare(sql)) {
            return Error(ErrorCode::QueryFailed, query.lastError().text());
        }

        for (size_t i = 0; i < params.size(); ++i) {
            query.bindValue(static_cast<int>(i), params[i]);
        }

        if (!query.exec()) {
            return Error(ErrorCode::QueryFailed, query.lastError().text());
        }

        m_lastInsertId = query.lastInsertId();
        return query.numRowsAffected();
    }

    Result<void> beginTransaction() override {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }
        if (!m_db.transaction()) {
            return Error(ErrorCode::DatabaseError, "Failed to begin transaction");
        }
        m_inTransaction = true;
        return {};
    }

    Result<void> commit() override {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }
        if (!m_db.commit()) {
            return Error(ErrorCode::DatabaseError, "Failed to commit transaction");
        }
        m_inTransaction = false;
        return {};
    }

    Result<void> rollback() override {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }
        if (!m_db.rollback()) {
            return Error(ErrorCode::DatabaseError, "Failed to rollback transaction");
        }
        m_inTransaction = false;
        return {};
    }

    bool isInTransaction() const override { return m_inTransaction; }
    QString getLastError() const override { return m_db.lastError().text(); }
    QString getConnectionId() const override { return QString::fromStdString(m_connectionId); }

    Result<QVariant> lastInsertId() override { return m_lastInsertId; }

protected:
    void generateConnectionId() {
        m_connectionId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    }

    QSqlDatabase m_db;
    std::string m_connectionId;
    bool m_inTransaction = false;
    QVariant m_lastInsertId;  ///< 最后一次 INSERT 的自增主键值 [v2.0.0]
};

class SqliteDriver : public BaseSqlDriver {
public:
    Result<void> open(const ConnectionConfig& config) override {
        if (isConnected()) {
            (void)close();
        }

        generateConnectionId();
        QString dbName = QString::fromStdString(m_connectionId);

        m_db = QSqlDatabase::addDatabase("QSQLITE", dbName);
        m_db.setDatabaseName(config.filePath);

        if (!m_db.open()) {
            return Error(ErrorCode::DatabaseError, m_db.lastError().text());
        }

        return {};
    }

    DatabaseType getType() const override { return DatabaseType::SQLite; }
};

std::unique_ptr<IDatabaseDriver> DatabaseDriverFactory::create(DatabaseType type) {
    switch (type) {
    case DatabaseType::SQLite:
        return std::make_unique<SqliteDriver>();
    case DatabaseType::MySQL:
        return std::make_unique<MysqlDriver>();
    case DatabaseType::PostgreSQL:
        return std::make_unique<PostgresDriver>();
    case DatabaseType::MSSQL:
        return nullptr;
    case DatabaseType::Oracle:
        return nullptr;
    default:
        return nullptr;
    }
}

} // namespace data
} // namespace sc
