#ifndef SOUL_ORM_SQLITE_REPOSITORY_H

#define SOUL_ORM_SQLITE_REPOSITORY_H



#include <exception>
#include <memory>

#include "soul/orm/base_repository.h"

#include "soul/orm/sql_dialect.h"

#include "soul/data/connection_pool.h"

#include "soul/data/database_driver.h"

#include "soul/core/uuid.h"

#include "soul/logging/logger.h"



namespace sc {

namespace orm {

namespace detail {
    class ConnectionGuard {
    public:
        ConnectionGuard(std::shared_ptr<data::DbConnectionPool> pool,
                        std::unique_ptr<data::IDatabaseDriver> conn)
            : m_pool(std::move(pool)), m_conn(std::move(conn)), m_released(false) {}

        ~ConnectionGuard() {
            if (!m_released && m_conn) {
                m_pool->release(std::move(m_conn));
            }
        }

        data::IDatabaseDriver* operator->() { return m_conn.get(); }
        data::IDatabaseDriver* get() { return m_conn.get(); }

    private:
        std::shared_ptr<data::DbConnectionPool> m_pool;
        std::unique_ptr<data::IDatabaseDriver> m_conn;
        bool m_released;
    };
} // namespace detail

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
    Result<int> count() override { return count(QueryWrapper()); }



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
        auto connResult = m_pool->acquire();
        if (!connResult.isOk()) {
            return Error(ErrorCode::DatabaseError, connResult.unwrapErr().message());
        }
        detail::ConnectionGuard guard(m_pool, std::move(connResult.unwrap()));

        QueryWrapper q = query;
        q.setDialect(m_dialect.get());
        QString sql = q.buildSelectSql(T::TABLE_NAME());
        auto result = guard->executeQuery(sql, q.getBindValues());

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }

        std::vector<T> entities;
        for (const auto& row : result.unwrap().rows) {
            entities.push_back(fromQueryResult(row));
        }
        return entities;
    } catch (const std::exception& e) {
        Logger::instance().error(QString("SqlRepository::find failed: %1").arg(e.what()), "orm");
        return Error(ErrorCode::DatabaseError, e.what());
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
    } catch (const std::exception& e) {
        Logger::instance().error(QString("SqlRepository::save failed: %1").arg(e.what()), "orm");
        return Error(ErrorCode::DatabaseError, e.what());
    }
}

template<typename T>
Result<void> SqlRepository<T>::remove(const QueryWrapper& query) {
    try {
        auto connResult = m_pool->acquire();
        if (!connResult.isOk()) {
            return Error(ErrorCode::DatabaseError, connResult.unwrapErr().message());
        }
        detail::ConnectionGuard guard(m_pool, std::move(connResult.unwrap()));

        QueryWrapper q = query;
        q.setDialect(m_dialect.get());
        QString sql = q.buildDeleteSql(T::TABLE_NAME());
        auto result = guard->executeUpdate(sql, q.getBindValues());

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }
        return {};
    } catch (const std::exception& e) {
        Logger::instance().error(QString("SqlRepository::remove failed: %1").arg(e.what()), "orm");
        return Error(ErrorCode::DatabaseError, e.what());
    }
}

template<typename T>
Result<int> SqlRepository<T>::count(const QueryWrapper& query) {
    try {
        auto connResult = m_pool->acquire();
        if (!connResult.isOk()) {
            return Error(ErrorCode::DatabaseError, connResult.unwrapErr().message());
        }
        detail::ConnectionGuard guard(m_pool, std::move(connResult.unwrap()));

        QueryWrapper q = query;
        q.setDialect(m_dialect.get());
        QString sql = q.buildCountSql(T::TABLE_NAME());
        auto result = guard->executeQuery(sql, q.getBindValues());

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }

        if (!result.unwrap().rows.empty()) {
            return result.unwrap().rows[0].begin()->second.toInt();
        }
        return 0;
    } catch (const std::exception& e) {
        Logger::instance().error(QString("SqlRepository::count failed: %1").arg(e.what()), "orm");
        return Error(ErrorCode::DatabaseError, e.what());
    }
}

template<typename T>
bool SqlRepository<T>::executeSql(const QString& sql, const std::vector<QVariant>& params) {
    try {
        auto connResult = m_pool->acquire();
        if (!connResult.isOk()) return false;
        detail::ConnectionGuard guard(m_pool, std::move(connResult.unwrap()));

        auto result = guard->executeUpdate(sql, params);
        return result.isOk();
    } catch (const std::exception& e) {
        Logger::instance().error(QString("SqlRepository::executeSql failed: %1").arg(e.what()), "orm");
        return false;
    }
}

template<typename T>
Result<T> SqlRepository<T>::insertInternal(const T& entity) {
    try {
        auto connResult = m_pool->acquire();
        if (!connResult.isOk()) {
            return Error(ErrorCode::DatabaseError, connResult.unwrapErr().message());
        }
        detail::ConnectionGuard guard(m_pool, std::move(connResult.unwrap()));

        QString sql = generateInsertSql(entity);
        auto params = collectInsertParams(entity);
        auto result = guard->executeUpdate(sql, params);

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }
        return entity;
    } catch (const std::exception& e) {
        Logger::instance().error(QString("SqlRepository::insertInternal failed: %1").arg(e.what()), "orm");
        return Error(ErrorCode::DatabaseError, e.what());
    }
}

template<typename T>
Result<T> SqlRepository<T>::updateInternal(const T& entity) {
    try {
        auto connResult = m_pool->acquire();
        if (!connResult.isOk()) {
            return Error(ErrorCode::DatabaseError, connResult.unwrapErr().message());
        }
        detail::ConnectionGuard guard(m_pool, std::move(connResult.unwrap()));

        QString sql = generateUpdateSql(entity);
        auto params = collectUpdateParams(entity);
        auto result = guard->executeUpdate(sql, params);

        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().message());
        }
        return entity;
    } catch (const std::exception& e) {
        Logger::instance().error(QString("SqlRepository::updateInternal failed: %1").arg(e.what()), "orm");
        return Error(ErrorCode::DatabaseError, e.what());
    }
}

template<typename T>
QString SqlRepository<T>::generateInsertSql(const T& entity) {
    auto meta = entity.getTableMeta();
    QStringList columns;
    QStringList placeholders;
    int paramIndex = 1;

    columns << "id";
    placeholders << m_dialect->convertPlaceholder(paramIndex);
    ++paramIndex;

    const QStringList baseFields = {"createTime", "updateTime", "deleted"};
    for (const auto& field : meta.fields) {
        if (field.second.isPrimaryKey) continue;
        if (baseFields.contains(field.first)) continue;
        columns << field.second.columnName;
        placeholders << m_dialect->convertPlaceholder(paramIndex);
        ++paramIndex;
    }

    columns << "create_time" << "update_time" << m_dialect->softDeleteConfig().columnName;
    placeholders << m_dialect->convertPlaceholder(paramIndex);
    ++paramIndex;
    placeholders << m_dialect->convertPlaceholder(paramIndex);
    ++paramIndex;
    placeholders << m_dialect->softDeleteConfig().logicNotDeletedValue;

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
                                              .arg(m_dialect->convertPlaceholder(paramIndex));
        ++paramIndex;
    }
    setClauses << QString("update_time = %1").arg(m_dialect->convertPlaceholder(paramIndex));
    ++paramIndex;

    QString idPlaceholder = m_dialect->convertPlaceholder(paramIndex);
    ++paramIndex;
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
