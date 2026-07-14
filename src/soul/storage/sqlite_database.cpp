#include "soul/storage/sqlite_database.h"
#include "soul/logging/log_macros.h"
#include <QUuid>
#include <QDir>

namespace sc {

SqliteDatabase::SqliteDatabase() {
    m_connectionName = QString("soul_sqlite_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

SqliteDatabase::~SqliteDatabase() {
    close();
}

Result<void> SqliteDatabase::open(const QString& filePath) {
    QMutexLocker locker(&m_mutex);

    if (m_db.isOpen()) {
        m_lastError = "Database already open";
        return Error(ErrorCode::InvalidArgument, "Database already open");
    }

    QString dir = QFileInfo(filePath).absolutePath();
    if (!QDir().mkpath(dir)) {
        m_lastError = QString("Cannot create directory: %1").arg(dir);
        return Error(ErrorCode::InternalError, m_lastError.toStdString());
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(filePath);

    if (!m_db.open()) {
        handleError(m_db.lastError());
        return Error(ErrorCode::InternalError, m_lastError.toStdString());
    }

    enableWAL();
    setBusyTimeout(5000);

    if (!m_encryptionKey.isEmpty()) {
        auto result = execute(QString("PRAGMA key = '%1';").arg(m_encryptionKey));
        if (result.isErr()) {
            return result.unwrapErr();
        }
    }

    SC_INFO("SQLite database opened: " + filePath.toStdString());
    return {};
}

void SqliteDatabase::close() {
    QMutexLocker locker(&m_mutex);

    if (m_db.isOpen()) {
        m_db.close();
        SC_INFO("SQLite database closed");
    }

    QSqlDatabase::removeDatabase(m_connectionName);
}

bool SqliteDatabase::isOpen() const {
    QMutexLocker locker(&m_mutex);
    return m_db.isOpen();
}

Result<QSqlQuery> SqliteDatabase::execute(const QString& sql) {
    return prepareAndExecute(sql);
}

Result<QSqlQuery> SqliteDatabase::execute(const QString& sql, const std::vector<QVariant>& params) {
    return prepareAndExecute(sql, params);
}

Result<int> SqliteDatabase::executeNonQuery(const QString& sql) {
    auto result = prepareAndExecute(sql);
    if (result.isErr()) {
        return result.unwrapErr();
    }
    return result.unwrap().numRowsAffected();
}

Result<int> SqliteDatabase::executeNonQuery(const QString& sql, const std::vector<QVariant>& params) {
    auto result = prepareAndExecute(sql, params);
    if (result.isErr()) {
        return result.unwrapErr();
    }
    return result.unwrap().numRowsAffected();
}

Result<QVariant> SqliteDatabase::executeScalar(const QString& sql) {
    auto result = prepareAndExecute(sql);
    if (result.isErr()) {
        return result.unwrapErr();
    }

    QSqlQuery query = result.unwrap();
    if (query.next()) {
        return query.value(0);
    }
    return QVariant();
}

Result<QVariant> SqliteDatabase::executeScalar(const QString& sql, const std::vector<QVariant>& params) {
    auto result = prepareAndExecute(sql, params);
    if (result.isErr()) {
        return result.unwrapErr();
    }

    QSqlQuery query = result.unwrap();
    if (query.next()) {
        return query.value(0);
    }
    return QVariant();
}

Result<void> SqliteDatabase::beginTransaction() {
    QMutexLocker locker(&m_mutex);

    if (!m_db.transaction()) {
        handleError(m_db.lastError());
        return Error(ErrorCode::InternalError, m_lastError.toStdString());
    }
    m_inTransaction = true;
    return {};
}

Result<void> SqliteDatabase::commitTransaction() {
    QMutexLocker locker(&m_mutex);

    if (!m_db.commit()) {
        handleError(m_db.lastError());
        return Error(ErrorCode::InternalError, m_lastError.toStdString());
    }
    m_inTransaction = false;
    return {};
}

Result<void> SqliteDatabase::rollbackTransaction() {
    QMutexLocker locker(&m_mutex);

    if (!m_db.rollback()) {
        handleError(m_db.lastError());
        return Error(ErrorCode::InternalError, m_lastError.toStdString());
    }
    m_inTransaction = false;
    return {};
}

bool SqliteDatabase::isInTransaction() const {
    QMutexLocker locker(&m_mutex);
    return m_inTransaction;
}

QString SqliteDatabase::lastError() const {
    QMutexLocker locker(&m_mutex);
    return m_lastError;
}

void SqliteDatabase::enableWAL(bool enabled) {
    QMutexLocker locker(&m_mutex);

    QString sql = enabled ? "PRAGMA journal_mode = WAL;" : "PRAGMA journal_mode = DELETE;";
    QSqlQuery query(sql, m_db);
    query.exec();

    if (enabled) {
        QSqlQuery walQuery("PRAGMA synchronous = NORMAL;", m_db);
        walQuery.exec();
    }
}

void SqliteDatabase::setBusyTimeout(int milliseconds) {
    QMutexLocker locker(&m_mutex);

    QString sql = QString("PRAGMA busy_timeout = %1;").arg(milliseconds);
    QSqlQuery query(sql, m_db);
    query.exec();
}

void SqliteDatabase::setEncryptionKey(const QString& key) {
    m_encryptionKey = key;
}

Result<QSqlQuery> SqliteDatabase::prepareAndExecute(const QString& sql, const std::vector<QVariant>& params) {
    QMutexLocker locker(&m_mutex);

    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        return Error(ErrorCode::InternalError, "Database not open");
    }

    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        handleError(query.lastError());
        return Error(ErrorCode::ParseError, m_lastError.toStdString());
    }

    for (size_t i = 0; i < params.size(); ++i) {
        query.bindValue(static_cast<int>(i), params[i]);
    }

    if (!query.exec()) {
        handleError(query.lastError());
        return Error(ErrorCode::InternalError, m_lastError.toStdString());
    }

    return query;
}

void SqliteDatabase::handleError(const QSqlError& error) {
    m_lastError = QString("%1 (%2)").arg(error.text()).arg(error.type());
    SC_ERROR("SQLite error: " + m_lastError.toStdString());
}

}