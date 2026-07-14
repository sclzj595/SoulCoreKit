#ifndef SOUL_STORAGE_SQLITE_DATABASE_H
#define SOUL_STORAGE_SQLITE_DATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QMutex>
#include <memory>
#include "soul/core/result.h"
#include "soul/core/error.h"

namespace sc {

class SqliteDatabase {
public:
    SqliteDatabase();
    ~SqliteDatabase();

    Result<void> open(const QString& filePath);
    void close();
    bool isOpen() const;

    Result<QSqlQuery> execute(const QString& sql);
    Result<QSqlQuery> execute(const QString& sql, const std::vector<QVariant>& params);
    Result<int> executeNonQuery(const QString& sql);
    Result<int> executeNonQuery(const QString& sql, const std::vector<QVariant>& params);
    Result<QVariant> executeScalar(const QString& sql);
    Result<QVariant> executeScalar(const QString& sql, const std::vector<QVariant>& params);

    Result<void> beginTransaction();
    Result<void> commitTransaction();
    Result<void> rollbackTransaction();
    bool isInTransaction() const;

    QString lastError() const;

    void enableWAL(bool enabled = true);
    void setBusyTimeout(int milliseconds);
    void setEncryptionKey(const QString& key);

private:
    mutable QMutex m_mutex;
    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_lastError;
    QString m_encryptionKey;
    bool m_inTransaction = false;

    Result<QSqlQuery> prepareAndExecute(const QString& sql, const std::vector<QVariant>& params = {});
    void handleError(const QSqlError& error);
};

}

#endif