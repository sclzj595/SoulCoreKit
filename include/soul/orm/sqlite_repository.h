#ifndef SOUL_ORM_SQLITE_REPOSITORY_H

#define SOUL_ORM_SQLITE_REPOSITORY_H



#include <memory>

#include "soul/orm/base_repository.h"

#include "soul/orm/sql_dialect.h"

#include "soul/data/connection_pool.h"

#include "soul/data/database_driver.h"

#include "soul/core/uuid.h"

#include "soul/logging/logger.h"



namespace sc {

namespace orm {



template<typename T>

class SqlRepository : public BaseRepository<T> {

public:

    SqlRepository(std::shared_ptr<data::DbConnectionPool> pool, SqlDialectType dialectType = SqlDialectType::SQLite);

    SqlRepository(std::shared_ptr<data::DbConnectionPool> pool, std::unique_ptr<ISqlDialect> dialect);

    ~SqlRepository() override;



    Result<std::vector<T>> find(const QueryWrapper& query) override;

    Result<T> save(const T& entity) override;

    Result<void> remove(const QueryWrapper& query) override;

    Result<int> count(const QueryWrapper& query) override;



    bool executeSql(const QString& sql, const std::vector<QVariant>& params = {}) override;



private:

    std::shared_ptr<data::DbConnectionPool> m_pool;
    std::unique_ptr<ISqlDialect> m_dialect;



    Result<T> insertInternal(const T& entity);

    Result<T> updateInternal(const T& entity);

    QString generateInsertSql(const T& entity);

    QString generateUpdateSql(const T& entity);

    std::vector<QVariant> collectInsertParams(const T& entity);

    std::vector<QVariant> collectUpdateParams(const T& entity);

    T fromQueryResult(const std::map<QString, QVariant>& row);

};

template<typename T>
SqlRepository<T>::SqlRepository(std::shared_ptr<data::DbConnectionPool> pool, SqlDialectType dialectType)
    : m_pool(std::move(pool)), m_dialect(ISqlDialect::create(dialectType)) {}

template<typename T>
SqlRepository<T>::SqlRepository(std::shared_ptr<data::DbConnectionPool> pool, std::unique_ptr<ISqlDialect> dialect)
    : m_pool(std::move(pool)), m_dialect(std::move(dialect)) {}

template<typename T>
SqlRepository<T>::~SqlRepository() {}

template<typename T>
Result<std::vector<T>> SqlRepository<T>::find(const QueryWrapper& query) {
    try {
        auto conn = m_pool->acquire();
        if (!conn.isOk()) {
            return Error(ErrorCode::DatabaseError, conn.unwrapErr().message());
        }

        QueryWrapper q = query;
        q.setDialect(m_dialect.get());
        QString sql = q.buildSelectSql(T::TABLE_NAME());
        auto result = conn.unwrap()->executeQuery(sql, q.getBindValues());
        m_pool->release(std::move(conn.unwrap()));

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }

        std::vector<T> entities;
        for (const auto& row : result.unwrap().rows) {
            entities.push_back(fromQueryResult(row));
        }
        return entities;
    } catch (...) {
        return Error(ErrorCode::DatabaseError, "Database exception");
    }
}

template<typename T>
Result<T> SqlRepository<T>::save(const T& entity) {
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
Result<void> SqlRepository<T>::remove(const QueryWrapper& query) {
    try {
        auto conn = m_pool->acquire();
        if (!conn.isOk()) {
            return Error(ErrorCode::DatabaseError, conn.unwrapErr().message());
        }

        QueryWrapper q = query;
        q.setDialect(m_dialect.get());
        QString sql = q.buildDeleteSql(T::TABLE_NAME());
        auto result = conn.unwrap()->executeUpdate(sql, q.getBindValues());
        m_pool->release(std::move(conn.unwrap()));

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }
        return {};
    } catch (...) {
        return Error(ErrorCode::DatabaseError, "Database exception");
    }
}

template<typename T>
Result<int> SqlRepository<T>::count(const QueryWrapper& query) {
    try {
        auto conn = m_pool->acquire();
        if (!conn.isOk()) {
            return Error(ErrorCode::DatabaseError, conn.unwrapErr().message());
        }

        QueryWrapper q = query;
        q.setDialect(m_dialect.get());
        QString sql = q.buildCountSql(T::TABLE_NAME());
        auto result = conn.unwrap()->executeQuery(sql, q.getBindValues());
        m_pool->release(std::move(conn.unwrap()));

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }

        if (!result.unwrap().rows.empty()) {
            return result.unwrap().rows[0].begin()->second.toInt();
        }
        return 0;
    } catch (...) {
        return Error(ErrorCode::DatabaseError, "Database exception");
    }
}

template<typename T>
bool SqlRepository<T>::executeSql(const QString& sql, const std::vector<QVariant>& params) {
    try {
        auto conn = m_pool->acquire();
        if (!conn.isOk()) return false;

        auto result = conn.unwrap()->executeUpdate(sql, params);
        m_pool->release(std::move(conn.unwrap()));
        return result.isOk();
    } catch (...) {
        return false;
    }
}

template<typename T>
Result<T> SqlRepository<T>::insertInternal(const T& entity) {
    try {
        auto conn = m_pool->acquire();
        if (!conn.isOk()) {
            return Error(ErrorCode::DatabaseError, conn.unwrapErr().message());
        }

        QString sql = generateInsertSql(entity);
        auto params = collectInsertParams(entity);
        auto result = conn.unwrap()->executeUpdate(sql, params);
        m_pool->release(std::move(conn.unwrap()));

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }
        return entity;
    } catch (...) {
        return Error(ErrorCode::DatabaseError, "Insert exception");
    }
}

template<typename T>
Result<T> SqlRepository<T>::updateInternal(const T& entity) {
    try {
        auto conn = m_pool->acquire();
        if (!conn.isOk()) {
            return Error(ErrorCode::DatabaseError, conn.unwrapErr().message());
        }

        QString sql = generateUpdateSql(entity);
        auto params = collectUpdateParams(entity);
        auto result = conn.unwrap()->executeUpdate(sql, params);
        m_pool->release(std::move(conn.unwrap()));

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }
        return entity;
    } catch (...) {
        return Error(ErrorCode::DatabaseError, "Update exception");
    }
}

template<typename T>
QString SqlRepository<T>::generateInsertSql(const T& entity) {
    auto meta = entity.getTableMeta();
    QStringList columns;
    QStringList placeholders;
    int paramIndex = 1;

    columns << "id";
    placeholders << m_dialect->convertPlaceholder(paramIndex++);

    const QStringList baseFields = {"createTime", "updateTime", "deleted"};
    for (const auto& field : meta.fields) {
        if (field.second.isPrimaryKey) continue;
        if (baseFields.contains(field.first)) continue;
        columns << field.second.columnName;
        placeholders << m_dialect->convertPlaceholder(paramIndex++);
    }

    columns << "create_time" << "update_time" << m_dialect->softDeleteConfig().columnName;
    placeholders << m_dialect->convertPlaceholder(paramIndex++)
                 << m_dialect->convertPlaceholder(paramIndex++)
                 << m_dialect->softDeleteConfig().logicNotDeletedValue;

    return QString("INSERT INTO %1(%2) VALUES (%3)")
        .arg(T::TABLE_NAME())
        .arg(columns.join(","))
        .arg(placeholders.join(","));
}

template<typename T>
QString SqlRepository<T>::generateUpdateSql(const T& entity) {
    auto meta = entity.getTableMeta();
    QStringList setClauses;
    int paramIndex = 1;

    const QStringList baseFields = {"createTime", "updateTime", "deleted"};
    for (const auto& field : meta.fields) {
        if (field.second.isPrimaryKey) continue;
        if (baseFields.contains(field.first)) continue;
        setClauses << QString("%1 = %2").arg(field.second.columnName)
                                              .arg(m_dialect->convertPlaceholder(paramIndex++));
    }
    setClauses << QString("update_time = %1").arg(m_dialect->convertPlaceholder(paramIndex++));

    QString idPlaceholder = m_dialect->convertPlaceholder(paramIndex++);
    return QString("UPDATE %1 SET %2 WHERE id = %3")
        .arg(T::TABLE_NAME())
        .arg(setClauses.join(","))
        .arg(idPlaceholder);
}

template<typename T>
std::vector<QVariant> SqlRepository<T>::collectInsertParams(const T& entity) {
    auto meta = entity.getTableMeta();
    std::vector<QVariant> params;

    params.push_back(entity.id);

    const QStringList baseFields = {"createTime", "updateTime", "deleted"};
    for (const auto& field : meta.fields) {
        if (field.second.isPrimaryKey) continue;
        if (baseFields.contains(field.first)) continue;
        params.push_back(entity.getProperty(field.first));
    }

    params.push_back(entity.createTime.toString(Qt::ISODate));
    params.push_back(entity.updateTime.toString(Qt::ISODate));

    return params;
}

template<typename T>
std::vector<QVariant> SqlRepository<T>::collectUpdateParams(const T& entity) {
    auto meta = entity.getTableMeta();
    std::vector<QVariant> params;

    const QStringList baseFields = {"createTime", "updateTime", "deleted"};
    for (const auto& field : meta.fields) {
        if (field.second.isPrimaryKey) continue;
        if (baseFields.contains(field.first)) continue;
        params.push_back(entity.getProperty(field.first));
    }

    params.push_back(entity.updateTime.toString(Qt::ISODate));
    params.push_back(entity.id);

    return params;
}

template<typename T>
T SqlRepository<T>::fromQueryResult(const std::map<QString, QVariant>& row) {
    T entity;

    auto it = row.find("id");
    if (it != row.end()) entity.id = it->second.toString();

    it = row.find("create_time");
    if (it != row.end()) entity.createTime = QDateTime::fromString(it->second.toString(), Qt::ISODate);

    it = row.find("update_time");
    if (it != row.end()) entity.updateTime = QDateTime::fromString(it->second.toString(), Qt::ISODate);

    it = row.find("deleted");
    if (it != row.end()) entity.deleted = it->second.toInt();

    auto meta = entity.getTableMeta();
    for (const auto& field : meta.fields) {
        it = row.find(field.second.columnName);
        if (it != row.end()) {
            entity.setProperty(field.first, it->second);
        }
    }

    return entity;
}

} // namespace orm
} // namespace sc



// Backward compatibility alias
namespace sc {
namespace orm {
template<typename T>
using SQLiteRepository = SqlRepository<T>;
}
}

#endif
