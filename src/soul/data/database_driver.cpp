#include "soul/data/database_driver.h"
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
            for (int i = 0; i < record.count(); ++i) {
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

protected:
    void generateConnectionId() {
        m_connectionId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    }

    QSqlDatabase m_db;
    std::string m_connectionId;
    bool m_inTransaction = false;
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

class MySqlDriver : public BaseSqlDriver {
public:
    Result<void> open(const ConnectionConfig& config) override {
        if (isConnected()) {
            (void)close();
        }

        generateConnectionId();
        QString dbName = QString::fromStdString(m_connectionId);

        m_db = QSqlDatabase::addDatabase("QMYSQL", dbName);
        m_db.setHostName(config.host);
        m_db.setPort(config.port);
        m_db.setDatabaseName(config.database);
        m_db.setUserName(config.username);
        m_db.setPassword(config.password);
        m_db.setConnectOptions(QString("connect_timeout=%1").arg(config.connectionTimeoutMs / 1000));

        if (!m_db.open()) {
            return Error(ErrorCode::DatabaseError, m_db.lastError().text());
        }

        return {};
    }

    DatabaseType getType() const override { return DatabaseType::MySQL; }
};

class PostgreSqlDriver : public BaseSqlDriver {
public:
    Result<void> open(const ConnectionConfig& config) override {
        if (isConnected()) {
            (void)close();
        }

        generateConnectionId();
        QString dbName = QString::fromStdString(m_connectionId);

        m_db = QSqlDatabase::addDatabase("QPSQL", dbName);
        m_db.setHostName(config.host);
        m_db.setPort(config.port);
        m_db.setDatabaseName(config.database);
        m_db.setUserName(config.username);
        m_db.setPassword(config.password);

        if (!m_db.open()) {
            return Error(ErrorCode::DatabaseError, m_db.lastError().text());
        }

        return {};
    }

    DatabaseType getType() const override { return DatabaseType::PostgreSQL; }
};

std::unique_ptr<IDatabaseDriver> DatabaseDriverFactory::create(DatabaseType type) {
    switch (type) {
    case DatabaseType::SQLite:
        return std::make_unique<SqliteDriver>();
    case DatabaseType::MySQL:
        return std::make_unique<MySqlDriver>();
    case DatabaseType::PostgreSQL:
        return std::make_unique<PostgreSqlDriver>();
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
