#ifndef SOUL_ORM_READ_WRITE_REPOSITORY_H
#define SOUL_ORM_READ_WRITE_REPOSITORY_H

#include <memory>
#include <QUuid>
#include "soul/orm/base_repository.h"
#include "soul/orm/sql_dialect.h"
#include "soul/data/database_driver.h"

namespace sc {
namespace orm {

template<typename T>
class IReadRepository {
public:
    virtual ~IReadRepository() = default;
    virtual Result<T> findById(const QString& id) = 0;
    virtual Result<std::vector<T>> findAll() = 0;
    virtual Result<std::vector<T>> find(const QueryWrapper& query) = 0;
    virtual Result<T> findOne(const QueryWrapper& query) = 0;
    virtual Result<int> count(const QueryWrapper& query = QueryWrapper()) = 0;
    virtual Result<bool> existsById(const QString& id) = 0;
};

template<typename T>
class IWriteRepository {
public:
    virtual ~IWriteRepository() = default;
    virtual Result<T> save(const T& entity) = 0;
    virtual Result<void> remove(const QueryWrapper& query) = 0;
    virtual Result<void> removeById(const QString& id) = 0;
    virtual Result<std::vector<T>> saveBatch(const std::vector<T>& entities) = 0;
    virtual bool executeSql(const QString& sql, const std::vector<QVariant>& params = {}) = 0;
};

template<typename T>
class ReadWriteRepository : public IReadRepository<T>, public IWriteRepository<T> {
public:
    ReadWriteRepository(
        std::shared_ptr<IDatabaseDriver> readDriver,
        std::shared_ptr<IDatabaseDriver> writeDriver,
        SqlDialectType dialectType = SqlDialectType::SQLite)
        : m_readDriver(readDriver)
        , m_writeDriver(writeDriver)
        , m_dialect(ISqlDialect::create(dialectType))
    {
    }

    ReadWriteRepository(
        std::shared_ptr<IDatabaseDriver> driver,
        SqlDialectType dialectType = SqlDialectType::SQLite)
        : m_readDriver(driver)
        , m_writeDriver(driver)
        , m_dialect(ISqlDialect::create(dialectType))
    {
    }

    void setDialect(std::unique_ptr<ISqlDialect> dialect) {
        m_dialect = std::move(dialect);
    }

    ISqlDialect* dialect() const { return m_dialect.get(); }

    Result<T> findById(const QString& id) override {
        QueryWrapper query;
        query.eq("id", id);
        return findOne(query);
    }

    Result<std::vector<T>> findAll() override {
        return find(QueryWrapper());
    }

    Result<std::vector<T>> find(const QueryWrapper& query) override {
        if (!m_readDriver || !m_readDriver->isConnected()) {
            return Error(ErrorCode::DatabaseError, "Read driver not connected");
        }

        QueryWrapper q = query;
        q.setDialect(m_dialect.get());
        QString sql = q.buildSelectSql(T::TABLE_NAME());
        auto bindValues = q.getBindValues();

        auto result = m_readDriver->executeQuery(sql, bindValues);
        if (!result.isOk()) {
            return result.unwrapErr();
        }

        std::vector<T> entities;
        auto queryResult = result.unwrap();
        entities.reserve(queryResult.rows.size());
        for (const auto& row : queryResult.rows) {
            entities.push_back(T::fromRow(row));
        }
        return entities;
    }

    Result<T> findOne(const QueryWrapper& query) override {
        QueryWrapper q = query;
        q.limit(1);
        auto result = find(q);
        if (!result.isOk()) {
            return result.unwrapErr();
        }
        auto list = result.unwrap();
        if (list.empty()) {
            return Error(ErrorCode::NotFound, "No record found");
        }
        return list[0];
    }

    Result<int> count(const QueryWrapper& query) override {
        if (!m_readDriver || !m_readDriver->isConnected()) {
            return Error(ErrorCode::DatabaseError, "Read driver not connected");
        }

        QueryWrapper q = query;
        q.setDialect(m_dialect.get());
        QString sql = q.buildCountSql(T::TABLE_NAME());
        auto bindValues = q.getBindValues();

        auto result = m_readDriver->executeQuery(sql, bindValues);
        if (!result.isOk()) {
            return result.unwrapErr();
        }

        auto queryResult = result.unwrap();
        if (queryResult.rows.empty()) {
            return 0;
        }
        return queryResult.rows[0].at("count").toInt();
    }

    Result<bool> existsById(const QString& id) override {
        auto result = findById(id);
        if (result.isOk()) return true;
        if (result.unwrapErr().code() == ErrorCode::NotFound) return false;
        return result.unwrapErr();
    }

    Result<T> save(const T& entity) override {
        if (!m_writeDriver || !m_writeDriver->isConnected()) {
            return Error(ErrorCode::DatabaseError, "Write driver not connected");
        }

        T entityCopy = entity;

        if (!entityCopy.id.isEmpty()) {
            entityCopy.beforeUpdate();
            TableMeta meta = entityCopy.getTableMeta();
            QString sql = generateUpdateSql(meta);
            auto params = collectUpdateParams(entityCopy);
            auto execResult = m_writeDriver->executeUpdate(sql, params);
            
            if (execResult.isOk() && execResult.unwrap() > 0) {
                return entityCopy;
            }
            if (!execResult.isOk()) {
                return execResult.unwrapErr();
            }
        }

        if (entityCopy.id.isEmpty()) {
            entityCopy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }

        entityCopy.beforeInsert();
        TableMeta meta = entityCopy.getTableMeta();
        QString sql = generateInsertSql(meta);
        auto params = collectInsertParams(entityCopy);
        auto execResult = m_writeDriver->executeUpdate(sql, params);
        if (!execResult.isOk()) {
            return execResult.unwrapErr();
        }
        return entityCopy;
    }

    Result<void> remove(const QueryWrapper& query) override {
        if (!m_writeDriver || !m_writeDriver->isConnected()) {
            return Error(ErrorCode::DatabaseError, "Write driver not connected");
        }

        QueryWrapper q = query;
        q.setDialect(m_dialect.get());
        QString sql = q.buildDeleteSql(T::TABLE_NAME());
        auto bindValues = q.getBindValues();

        auto result = m_writeDriver->executeUpdate(sql, bindValues);
        if (!result.isOk()) {
            return result.unwrapErr();
        }
        return Ok();
    }

    Result<void> removeById(const QString& id) override {
        QueryWrapper query;
        query.eq("id", id);
        return remove(query);
    }

    Result<std::vector<T>> saveBatch(const std::vector<T>& entities) override {
        std::vector<T> saved;
        saved.reserve(entities.size());
        for (const auto& entity : entities) {
            auto result = save(entity);
            if (!result.isOk()) {
                return result.unwrapErr();
            }
            saved.push_back(result.unwrap());
        }
        return saved;
    }

    bool executeSql(const QString& sql, const std::vector<QVariant>& params) override {
        if (!m_writeDriver || !m_writeDriver->isConnected()) {
            return false;
        }
        auto result = m_writeDriver->executeUpdate(sql, params);
        return result.isOk();
    }

private:
    QString generateInsertSql(const TableMeta& meta) {
        QStringList columns;
        QStringList placeholders;
        int paramIndex = 1;

        columns << "id";
        placeholders << m_dialect->convertPlaceholder(paramIndex);
        ++paramIndex;

        const QStringList baseFields = {"createTime", "updateTime", "deleted"};
        for (const auto& [name, field] : meta.fields) {
            if (field.isPrimaryKey) continue;
            if (baseFields.contains(name)) continue;
            columns << field.columnName;
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

    std::vector<QVariant> collectInsertParams(const T& entity) {
        TableMeta meta = entity.getTableMeta();
        std::vector<QVariant> params;

        params.push_back(entity.id);

        const QStringList baseFields = {"createTime", "updateTime", "deleted"};
        for (const auto& [name, field] : meta.fields) {
            if (field.isPrimaryKey) continue;
            if (baseFields.contains(name)) continue;
            params.push_back(entity.getProperty(name));
        }

        params.push_back(entity.createTime.toString(Qt::ISODate));
        params.push_back(entity.updateTime.toString(Qt::ISODate));

        return params;
    }

    QString generateUpdateSql(const TableMeta& meta) {
        int paramIdx = 0;
        QStringList numberedClauses;
        const QStringList baseFields = {"createTime", "updateTime", "deleted"};
        for (const auto& [name, field] : meta.fields) {
            if (field.isPrimaryKey) continue;
            if (baseFields.contains(name)) continue;
            numberedClauses << field.columnName + " = " + m_dialect->convertPlaceholder(++paramIdx);
        }

        numberedClauses << "update_time = " + m_dialect->convertPlaceholder(++paramIdx);

        return QString("UPDATE %1 SET %2 WHERE id = %3")
            .arg(T::TABLE_NAME())
            .arg(numberedClauses.join(", "))
            .arg(m_dialect->convertPlaceholder(++paramIdx));
    }

    std::vector<QVariant> collectUpdateParams(const T& entity) {
        TableMeta meta = entity.getTableMeta();
        std::vector<QVariant> params;

        const QStringList baseFields = {"createTime", "updateTime", "deleted"};
        for (const auto& [name, field] : meta.fields) {
            if (field.isPrimaryKey) continue;
            if (baseFields.contains(name)) continue;
            params.push_back(entity.getProperty(name));
        }

        params.push_back(entity.updateTime.toString(Qt::ISODate));
        params.push_back(entity.id);

        return params;
    }

    std::shared_ptr<IDatabaseDriver> m_readDriver;
    std::shared_ptr<IDatabaseDriver> m_writeDriver;
    std::unique_ptr<ISqlDialect> m_dialect;
};

} // namespace orm
} // namespace sc

#endif
