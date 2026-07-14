#ifndef SOUL_STORAGE_REPOSITORY_H
#define SOUL_STORAGE_REPOSITORY_H

#include <vector>
#include <string>
#include <functional>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include "soul/core/result.h"
#include "soul/core/error.h"

namespace sc {

/**
 * @class IRepository
 * @brief 数据仓库接口定义
 *
 * IRepository 定义了统一的数据访问接口，支持基本的 CRUD 操作：
 * - findById：根据 ID 查询单个实体
 * - findAll：查询所有实体
 * - save：保存实体（新增或更新）
 * - remove：根据 ID 删除实体
 *
 * @tparam T 实体类型
 * @see MemoryRepository, SqliteRepository
 */
template<typename T>
class IRepository {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IRepository() = default;

    /**
     * @brief 根据 ID 查询实体
     * @param id 实体 ID
     * @return 包含实体的 Result，如果未找到返回 NotFound 错误
     */
    virtual Result<T> findById(const QString& id) = 0;

    /**
     * @brief 查询所有实体
     * @return 包含实体列表的 Result
     */
    virtual Result<std::vector<T>> findAll() = 0;

    /**
     * @brief 保存实体（新增或更新）
     * @param entity 待保存的实体
     * @return ResultVoid，如果失败返回错误
     */
    virtual Result<void> save(const T& entity) = 0;

    /**
     * @brief 根据 ID 删除实体
     * @param id 实体 ID
     * @return ResultVoid，如果未找到返回 NotFound 错误
     */
    virtual Result<void> remove(const QString& id) = 0;
};

/**
 * @class MemoryRepository
 * @brief 内存数据仓库实现
 *
 * MemoryRepository 是 IRepository 的内存实现，数据存储在内存中，
 * 进程退出后数据丢失。适用于临时数据存储或测试场景。
 *
 * 使用方式：
 * @code
 * class MyEntityRepository : public MemoryRepository<MyEntity> {
 *     QString getId(const MyEntity& entity) const override {
 *         return entity.id();
 *     }
 * };
 *
 * MyEntityRepository repo;
 * repo.save(entity);
 * auto result = repo.findById("123");
 * @endcode
 *
 * @tparam T 实体类型
 * @see IRepository, SqliteRepository
 */
template<typename T>
class MemoryRepository : public IRepository<T> {
public:
    /**
     * @brief 根据 ID 查询实体
     * @param id 实体 ID
     * @return 包含实体的 Result，如果未找到返回 NotFound 错误
     */
    Result<T> findById(const QString& id) override {
        for (const auto& entity : m_data) {
            if (getId(entity) == id) {
                return entity;
            }
        }
        return Error(ErrorCode::NotFound, "Entity not found");
    }

    /**
     * @brief 查询所有实体
     * @return 包含实体列表的 Result
     */
    Result<std::vector<T>> findAll() override {
        return m_data;
    }

    /**
     * @brief 保存实体（新增或更新）
     * @param entity 待保存的实体
     * @return ResultVoid
     */
    Result<void> save(const T& entity) override {
        auto it = std::find_if(m_data.begin(), m_data.end(),
            [this, &entity](const T& e) { return getId(e) == getId(entity); });

        if (it != m_data.end()) {
            *it = entity;
        } else {
            m_data.push_back(entity);
        }
        return {};
    }

    /**
     * @brief 根据 ID 删除实体
     * @param id 实体 ID
     * @return ResultVoid，如果未找到返回 NotFound 错误
     */
    Result<void> remove(const QString& id) override {
        auto it = std::remove_if(m_data.begin(), m_data.end(),
            [this, &id](const T& e) { return getId(e) == id; });

        if (it != m_data.end()) {
            m_data.erase(it, m_data.end());
            return {};
        }
        return Error(ErrorCode::NotFound, "Entity not found");
    }

    /**
     * @brief 获取实体的 ID（纯虚函数，子类必须实现）
     * @param entity 实体对象
     * @return 实体 ID
     */
    virtual QString getId(const T& entity) const = 0;

private:
    std::vector<T> m_data;
};

/**
 * @class SqliteRepository
 * @brief SQLite 数据仓库实现
 *
 * SqliteRepository 是 IRepository 的 SQLite 实现，数据持久化存储在
 * SQLite 数据库文件中。支持自定义实体到行的映射和行到实体的映射。
 *
 * 使用方式：
 * @code
 * SqliteRepository<MyEntity> repo(
 *     "data.db",
 *     "my_table",
 *     [](const MyEntity& e) {
 *         QVariantMap row;
 *         row["id"] = e.id();
 *         row["name"] = e.name();
 *         return row;
 *     },
 *     [](const QVariantMap& row) {
 *         MyEntity e;
 *         e.setId(row["id"].toString());
 *         e.setName(row["name"].toString());
 *         return e;
 *     }
 * );
 *
 * repo.save(entity);
 * auto result = repo.findById("123");
 * @endcode
 *
 * @tparam T 实体类型
 * @see IRepository, MemoryRepository
 */
template<typename T>
class SqliteRepository : public IRepository<T> {
public:
    /**
     * @brief 实体到行的映射函数类型
     */
    using EntityToRow = std::function<QVariantMap(const T&)>;

    /**
     * @brief 行到实体的映射函数类型
     */
    using RowToEntity = std::function<T(const QVariantMap&)>;

    /**
     * @brief 构造函数
     * @param dbPath 数据库文件路径
     * @param tableName 表名
     * @param entityToRow 实体到行的映射函数
     * @param rowToEntity 行到实体的映射函数
     * @param idColumn ID 列名（默认为 "id"）
     */
    SqliteRepository(const QString& dbPath, const QString& tableName,
                     EntityToRow entityToRow, RowToEntity rowToEntity,
                     const QString& idColumn = "id")
        : m_dbPath(dbPath), m_tableName(tableName),
          m_entityToRow(std::move(entityToRow)), m_rowToEntity(std::move(rowToEntity)),
          m_idColumn(idColumn) {
    }

    /**
     * @brief 根据 ID 查询实体
     * @param id 实体 ID
     * @return 包含实体的 Result，如果未找到返回 NotFound 错误
     */
    Result<T> findById(const QString& id) override {
        QSqlDatabase db = getDatabase();
        if (!db.isOpen()) {
            return Error(ErrorCode::StorageError, "Database not open");
        }

        QSqlQuery query(db);
        QString sql = QString("SELECT * FROM %1 WHERE %2 = ?").arg(m_tableName).arg(m_idColumn);
        query.prepare(sql);
        query.addBindValue(id);

        if (!query.exec()) {
            return Error(ErrorCode::StorageError, query.lastError().text().toStdString());
        }

        if (!query.next()) {
            return Error(ErrorCode::NotFound, "Entity not found");
        }

        return m_rowToEntity(queryToMap(query));
    }

    /**
     * @brief 查询所有实体
     * @return 包含实体列表的 Result
     */
    Result<std::vector<T>> findAll() override {
        QSqlDatabase db = getDatabase();
        if (!db.isOpen()) {
            return Error(ErrorCode::StorageError, "Database not open");
        }

        QSqlQuery query(db);
        QString sql = QString("SELECT * FROM %1").arg(m_tableName);

        if (!query.exec()) {
            return Error(ErrorCode::StorageError, query.lastError().text().toStdString());
        }

        std::vector<T> results;
        while (query.next()) {
            results.push_back(m_rowToEntity(queryToMap(query)));
        }

        return results;
    }

    /**
     * @brief 保存实体（新增或更新）
     * @param entity 待保存的实体
     * @return ResultVoid，如果失败返回错误
     */
    Result<void> save(const T& entity) override {
        QSqlDatabase db = getDatabase();
        if (!db.isOpen()) {
            return Error(ErrorCode::StorageError, "Database not open");
        }

        QString id = getId(entity);
        QSqlQuery checkQuery(db);
        QString checkSql = QString("SELECT COUNT(*) FROM %1 WHERE %2 = ?").arg(m_tableName).arg(m_idColumn);
        checkQuery.prepare(checkSql);
        checkQuery.addBindValue(id);

        if (!checkQuery.exec()) {
            return Error(ErrorCode::StorageError, checkQuery.lastError().text().toStdString());
        }

        checkQuery.next();
        bool exists = checkQuery.value(0).toInt() > 0;

        QVariantMap row = m_entityToRow(entity);

        if (exists) {
            if (!updateEntity(id, row)) {
                return Error(ErrorCode::StorageError, "Update failed");
            }
        } else {
            if (!insertEntity(row)) {
                return Error(ErrorCode::StorageError, "Insert failed");
            }
        }

        return {};
    }

    /**
     * @brief 根据 ID 删除实体
     * @param id 实体 ID
     * @return ResultVoid，如果未找到返回 NotFound 错误
     */
    Result<void> remove(const QString& id) override {
        QSqlDatabase db = getDatabase();
        if (!db.isOpen()) {
            return Error(ErrorCode::StorageError, "Database not open");
        }

        QSqlQuery query(db);
        QString sql = QString("DELETE FROM %1 WHERE %2 = ?").arg(m_tableName).arg(m_idColumn);
        query.prepare(sql);
        query.addBindValue(id);

        if (!query.exec()) {
            return Error(ErrorCode::StorageError, query.lastError().text().toStdString());
        }

        if (query.numRowsAffected() == 0) {
            return Error(ErrorCode::NotFound, "Entity not found");
        }

        return {};
    }

    /**
     * @brief 获取实体的 ID（纯虚函数，子类必须实现）
     * @param entity 实体对象
     * @return 实体 ID
     */
    virtual QString getId(const T& entity) const = 0;

private:
    /**
     * @brief 获取数据库连接
     * @return QSqlDatabase 对象
     */
    QSqlDatabase getDatabase() {
        QString connectionName = "sqlite_repo_" + m_dbPath;

        if (QSqlDatabase::contains(connectionName)) {
            return QSqlDatabase::database(connectionName);
        }

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(m_dbPath);

        if (!db.open()) {
            return db;
        }

        return db;
    }

    /**
     * @brief 将查询结果转换为 QVariantMap
     * @param query SQL 查询对象
     * @return 包含列名和值的映射
     */
    QVariantMap queryToMap(const QSqlQuery& query) {
        QVariantMap map;
        QSqlRecord record = query.record();

        for (int i = 0; i < record.count(); ++i) {
            map[record.fieldName(i)] = query.value(i);
        }

        return map;
    }

    /**
     * @brief 插入实体
     * @param row 实体数据
     * @return 如果成功返回 true
     */
    bool insertEntity(const QVariantMap& row) {
        QSqlDatabase db = getDatabase();
        QSqlQuery query(db);

        QStringList columns;
        QStringList placeholders;
        QList<QVariant> values;

        for (auto it = row.begin(); it != row.end(); ++it) {
            columns << it.key();
            placeholders << "?";
            values << it.value();
        }

        QString sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
                          .arg(m_tableName)
                          .arg(columns.join(", "))
                          .arg(placeholders.join(", "));

        query.prepare(sql);
        for (const QVariant& value : values) {
            query.addBindValue(value);
        }

        return query.exec();
    }

    /**
     * @brief 更新实体
     * @param id 实体 ID
     * @param row 实体数据
     * @return 如果成功返回 true
     */
    bool updateEntity(const QString& id, const QVariantMap& row) {
        QSqlDatabase db = getDatabase();
        QSqlQuery query(db);

        QStringList setClauses;
        QList<QVariant> values;

        for (auto it = row.begin(); it != row.end(); ++it) {
            if (it.key() != m_idColumn) {
                setClauses << QString("%1 = ?").arg(it.key());
                values << it.value();
            }
        }

        values << id;

        QString sql = QString("UPDATE %1 SET %2 WHERE %3 = ?")
                          .arg(m_tableName)
                          .arg(setClauses.join(", "))
                          .arg(m_idColumn);

        query.prepare(sql);
        for (const QVariant& value : values) {
            query.addBindValue(value);
        }

        return query.exec();
    }

    QString m_dbPath;
    QString m_tableName;
    QString m_idColumn;
    EntityToRow m_entityToRow;
    RowToEntity m_rowToEntity;
};

}

#endif
