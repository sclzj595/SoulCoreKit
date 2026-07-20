#ifndef SOUL_ORM_SQLITE_REPOSITORY_H
#define SOUL_ORM_SQLITE_REPOSITORY_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include <memory>
#include "soul/orm/base_repository.h"
#include "soul/core/uuid.h"
#include "soul/logging/logger.h"

namespace sc {
namespace orm {

template<typename T>
class SQLiteRepository : public BaseRepository<T> {
public:
    SQLiteRepository(const QString& dbPath);
    ~SQLiteRepository() override;

    Result<T> findById(const QString& id) override;
    Result<std::vector<T>> findAll() override;
    Result<std::vector<T>> find(const QueryWrapper& query) override;
    Result<T> save(const T& entity) override;
    Result<void> removeById(const QString& id) override;
    Result<void> remove(const QueryWrapper& query) override;
    Result<int> count() override;
    Result<int> count(const QueryWrapper& query) override;

    bool executeSql(const QString& sql, const std::vector<QVariant>& params = {}) override;
    Result<std::vector<T>> saveBatch(const std::vector<T>& entities) override;

private:
    QSqlDatabase m_db;
    mutable QMutex m_mutex;
    bool m_initialized = false;

    Result<T> insertInternal(const T& entity);
    Result<T> updateInternal(const T& entity);
    QString generateInsertSql(const T& entity);
    QString generateUpdateSql(const T& entity);
    void bindEntityValues(QSqlQuery& q, const T& entity);
    std::map<QString, QVariant> extractEntityValues(const T& entity);
    T fromSql(const QSqlQuery& q);
};

template<typename T>
SQLiteRepository<T>::SQLiteRepository(const QString& dbPath) {
    try {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "orm_db_" + QString::fromStdString(sc::Uuid::generate()));
        m_db.setDatabaseName(dbPath);
        if (m_db.open()) {
            m_db.exec("PRAGMA foreign_keys = ON");
            m_initialized = true;
        } else {
            Logger::instance().error(m_db.lastError().text().toStdString(), "SQLiteRepository");
        }
    } catch (...) {
        Logger::instance().error("Failed to initialize SQLiteRepository", "SQLiteRepository");
    }
}

template<typename T>
SQLiteRepository<T>::~SQLiteRepository() {
    if (!m_initialized) return;
    QString connectionName = m_db.connectionName();
    try {
        if (m_db.isOpen()) {
            m_db.close();
        }
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
    } catch (...) {
        Logger::instance().error("Error during SQLiteRepository destruction", "SQLiteRepository");
    }
}

template<typename T>
Result<T> SQLiteRepository<T>::findById(const QString& id) {
    QueryWrapper query;
    query.eq("id", id);
    return this->findOne(query);
}

template<typename T>
Result<std::vector<T>> SQLiteRepository<T>::findAll() {
    QueryWrapper query;
    return find(query);
}

template<typename T>
Result<std::vector<T>> SQLiteRepository<T>::find(const QueryWrapper& query) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return Error(ErrorCode::DatabaseError, "Database not initialized");
    }
    try {
        QString sql = query.buildSelectSql(T::TABLE_NAME());
        QSqlQuery q(m_db);
        q.prepare(sql);
        auto values = query.getBindValues();
        for (size_t i = 0; i < values.size(); ++i) {
            q.bindValue(static_cast<int>(i), values[i]);
        }

        if (!q.exec()) {
            return Error(ErrorCode::DatabaseError, q.lastError().text());
        }

        std::vector<T> result;
        while (q.next()) {
            T entity = fromSql(q);
            result.push_back(entity);
        }
        return result;
    } catch (...) {
        return Error(ErrorCode::DatabaseError, "Database exception");
    }
}

template<typename T>
Result<T> SQLiteRepository<T>::save(const T& entity) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return Error(ErrorCode::DatabaseError, "Database not initialized");
    }
    try {
        T e = entity;
        if (e.id.isEmpty()) {
            e.id = QString::fromStdString(sc::Uuid::generate());
            e.beforeInsert();
            return insertInternal(e);
        } else {
            e.beforeUpdate();
            return updateInternal(e);
        }
    } catch (...) {
        return Error(ErrorCode::DatabaseError, "Database exception");
    }
}

template<typename T>
Result<void> SQLiteRepository<T>::removeById(const QString& id) {
    QueryWrapper query;
    query.eq("id", id);
    return remove(query);
}

template<typename T>
Result<void> SQLiteRepository<T>::remove(const QueryWrapper& query) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return Error(ErrorCode::DatabaseError, "Database not initialized");
    }
    try {
        QString sql = query.buildDeleteSql(T::TABLE_NAME());
        QSqlQuery q(m_db);
        q.prepare(sql);
        auto values = query.getBindValues();
        for (size_t i = 0; i < values.size(); ++i) {
            q.bindValue(static_cast<int>(i), values[i]);
        }

        if (!q.exec()) {
            return Error(ErrorCode::DatabaseError, q.lastError().text());
        }
        return {};
    } catch (...) {
        return Error(ErrorCode::DatabaseError, "Database exception");
    }
}

template<typename T>
Result<int> SQLiteRepository<T>::count() {
    QueryWrapper query;
    return count(query);
}

template<typename T>
Result<int> SQLiteRepository<T>::count(const QueryWrapper& query) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return Error(ErrorCode::DatabaseError, "Database not initialized");
    }
    try {
        QString sql = query.buildCountSql(T::TABLE_NAME());
        QSqlQuery q(m_db);
        q.prepare(sql);
        auto values = query.getBindValues();
        for (size_t i = 0; i < values.size(); ++i) {
            q.bindValue(static_cast<int>(i), values[i]);
        }

        if (!q.exec()) {
            return Error(ErrorCode::DatabaseError, q.lastError().text());
        }

        if (q.next()) {
            return q.value(0).toInt();
        }
        return 0;
    } catch (...) {
        return Error(ErrorCode::DatabaseError, "Database exception");
    }
}

template<typename T>
bool SQLiteRepository<T>::executeSql(const QString& sql, const std::vector<QVariant>& params) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) return false;
    QSqlQuery q(m_db);
    q.prepare(sql);
    for (size_t i = 0; i < params.size(); ++i) {
        q.bindValue(static_cast<int>(i), params[i]);
    }
    return q.exec();
}

template<typename T>
Result<std::vector<T>> SQLiteRepository<T>::saveBatch(const std::vector<T>& entities) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return Error(ErrorCode::DatabaseError, "Database not initialized");
    }
    try {
        m_db.transaction();
        std::vector<T> saved;
        for (const auto& entity : entities) {
            T e = entity;
            if (e.id.isEmpty()) {
                e.id = QString::fromStdString(sc::Uuid::generate());
                e.beforeInsert();
                auto result = insertInternal(e);
                if (!result.isOk()) {
                    m_db.rollback();
                    return result.unwrapErr();
                }
                saved.push_back(result.unwrap());
            } else {
                e.beforeUpdate();
                auto result = updateInternal(e);
                if (!result.isOk()) {
                    m_db.rollback();
                    return result.unwrapErr();
                }
                saved.push_back(result.unwrap());
            }
        }
        m_db.commit();
        return saved;
    } catch (...) {
        m_db.rollback();
        return Error(ErrorCode::DatabaseError, "Batch save exception");
    }
}

template<typename T>
Result<T> SQLiteRepository<T>::insertInternal(const T& entity) {
    QString sql = generateInsertSql(entity);
    QSqlQuery q(m_db);
    q.prepare(sql);
    bindEntityValues(q, entity);

    if (!q.exec()) {
        return Error(ErrorCode::DatabaseError, q.lastError().text());
    }
    return entity;
}

template<typename T>
Result<T> SQLiteRepository<T>::updateInternal(const T& entity) {
    auto meta = entity.getTableMeta();
    QString sql = generateUpdateSql(entity);
    QSqlQuery q(m_db);
    q.prepare(sql);

    int idx = 0;
    for (const auto& pair : extractEntityValues(entity)) {
        q.bindValue(idx++, pair.second);
    }
    q.bindValue(idx++, entity.id);

    if (!q.exec()) {
        return Error(ErrorCode::DatabaseError, q.lastError().text());
    }
    return entity;
}

template<typename T>
QString SQLiteRepository<T>::generateInsertSql(const T& entity) {
    auto meta = entity.getTableMeta();
    QStringList columns;
    QStringList placeholders;

    columns << "id";
    placeholders << "?";

    for (const auto& field : meta.fields) {
        if (field.second.isPrimaryKey) continue;
        columns << field.second.columnName;
        placeholders << "?";
    }

    columns << "create_time" << "update_time" << "deleted";
    placeholders << "?" << "?" << "0";

    return QString("INSERT INTO %1(%2) VALUES (%3)")
        .arg(T::TABLE_NAME())
        .arg(columns.join(","))
        .arg(placeholders.join(","));
}

template<typename T>
QString SQLiteRepository<T>::generateUpdateSql(const T& entity) {
    auto meta = entity.getTableMeta();
    QStringList setClauses;

    for (const auto& field : meta.fields) {
        if (field.second.isPrimaryKey) continue;
        setClauses << QString("%1 = ?").arg(field.second.columnName);
    }
    setClauses << "update_time = ?";

    return QString("UPDATE %1 SET %2 WHERE id = ?")
        .arg(T::TABLE_NAME())
        .arg(setClauses.join(","));
}

template<typename T>
void SQLiteRepository<T>::bindEntityValues(QSqlQuery& q, const T& entity) {
    auto meta = entity.getTableMeta();
    int idx = 0;

    q.bindValue(idx++, entity.id);

    for (const auto& field : meta.fields) {
        if (field.second.isPrimaryKey) continue;
        q.bindValue(idx++, entity.getProperty(field.first));
    }

    q.bindValue(idx++, entity.createTime.toString(Qt::ISODate));
    q.bindValue(idx++, entity.updateTime.toString(Qt::ISODate));
}

template<typename T>
std::map<QString, QVariant> SQLiteRepository<T>::extractEntityValues(const T& entity) {
    auto meta = entity.getTableMeta();
    std::map<QString, QVariant> values;

    for (const auto& field : meta.fields) {
        if (field.second.isPrimaryKey) continue;
        values[field.second.columnName] = entity.getProperty(field.first);
    }

    return values;
}

template<typename T>
T SQLiteRepository<T>::fromSql(const QSqlQuery& q) {
    T entity;
    auto meta = entity.getTableMeta();

    entity.id = q.value("id").toString();
    entity.createTime = QDateTime::fromString(q.value("create_time").toString(), Qt::ISODate);
    entity.updateTime = QDateTime::fromString(q.value("update_time").toString(), Qt::ISODate);
    entity.deleted = q.value("deleted").toInt();

    for (const auto& field : meta.fields) {
        entity.setProperty(field.first, q.value(field.second.columnName));
    }

    return entity;
}

} // namespace orm
} // namespace sc

#endif