#ifndef SOUL_DATA_BASE_REPOSITORY_H
#define SOUL_DATA_BASE_REPOSITORY_H

// ============================================================================
// base_repository.h — MyBatis-Plus 风格通用 CRUD 基类 [v2.0.0 新增]
// ============================================================================
//
// 对标 MyBatis-Plus 的 BaseMapper<T>，提供开箱即用的 CRUD 操作。
// 通过 IDatabaseDriver 多态封装，支持 SQLite / MySQL / PostgreSQL 等任意数据库。
//
// 核心设计:
//   - BaseRepository<Entity> 提供 insert/delete/update/select 全套 CRUD
//   - 通过 EntityTraits 自动检测表名和主键
//   - 通过 ReflectiveEntity 反射实现行 → 实体映射
//   - 通过 IDatabaseDriver 多态支持任意数据库驱动
//   - 内置 QueryBuilder 访问器，支持复杂条件查询
//
// 用法:
//   // 1. 定义实体
//   class User : public ReflectiveEntity {
//   public:
//       User() {
//           SC_PROPERTY(User, id, getId, setId);
//           SC_PROPERTY(User, name, getName, setName);
//           SC_PROPERTY(User, email, getEmail, setEmail);
//       }
//       SC_TABLE("users")
//       SC_PRIMARY_KEY("id")
//       int getId() const { return m_id; }
//       void setId(int v) { m_id = v; }
//       QString getName() const { return m_name; }
//       void setName(const QString& v) { m_name = v; }
//       QString getEmail() const { return m_email; }
//       void setEmail(const QString& v) { m_email = v; }
//   private:
//       int m_id = 0;
//       QString m_name;
//       QString m_email;
//   };
//
//   // 2. 创建 Repository（继承即用）
//   class UserRepository : public BaseRepository<User> {
//   public:
//       using BaseRepository::BaseRepository; // 继承构造函数
//       // 自定义查询方法
//       Result<std::vector<User>> findByEmail(const QString& email) {
//           return selectByCondition("email = ?", {email});
//       }
//   };
//
//   // 3. 使用
//   auto driver = DatabaseDriverFactory::instance().create(config);
//   UserRepository repo(driver);
//   auto result = repo.selectById(1);
//   auto users = repo.selectList();
//   auto page = repo.selectPage(1, 10);

#include <QString>
#include <QVariant>
#include <QStringList>
#include <memory>
#include <type_traits>
#include <vector>
#include <map>

#include "soul/data/database_driver.h"
#include "soul/data/orm_reflection.h"
#include "soul/data/query_builder.h"
#include "soul/core/result.h"
#include "soul/core/error.h"

namespace sc {
namespace data {

// ============================================================================
// EntityTraits<T> — 实体元数据检测
// ============================================================================
//
// 通过 SFINAE 检测实体类是否定义了 SC_TABLE / SC_PRIMARY_KEY 宏。
// 如果定义了则使用，否则使用默认值（类名 + "id"）。
template<typename T>
struct EntityTraits {
private:
    // 检测 _sc_table_name() 成员函数
    template<typename U>
    static auto detectTableName(int) -> decltype(U::_sc_table_name(), QString()) {
        return U::_sc_table_name();
    }
    template<typename U>
    static auto detectTableName(...) -> QString {
        // 默认: 使用类名作为表名
        return QString::fromStdString(typeid(T).name());
    }

    // 检测 _sc_primary_key() 成员函数
    template<typename U>
    static auto detectPrimaryKey(int) -> decltype(U::_sc_primary_key(), QString()) {
        return U::_sc_primary_key();
    }
    template<typename U>
    static auto detectPrimaryKey(...) -> QString {
        return QStringLiteral("id");
    }

public:
    static QString tableName() { return detectTableName<T>(0); }
    static QString primaryKey() { return detectPrimaryKey<T>(0); }
};

// ============================================================================
// BaseRepository<Entity> — 通用 CRUD 仓库基类
// ============================================================================
//
// @tparam Entity 实体类型，必须继承自 ReflectiveEntity 且提供默认构造函数
//
// 内置 CRUD 方法:
//   - insert(entity)        插入单条记录（自动回填自增主键）
//   - insertBatch(entities) 批量插入
//   - deleteById(id)        按主键删除
//   - deleteByCondition(where, params) 按条件删除
//   - updateById(entity)    按主键更新
//   - updateByCondition(entity, where, params) 按条件更新
//   - selectById(id)        按主键查询
//   - selectList()          查询全部
//   - selectByCondition(where, params) 按条件查询
//   - selectPage(page, size) 分页查询
//   - count()               统计总数
//   - exists(id)            检查是否存在
//
// 查询构建器:
//   - queryBuilder()        获取 QueryBuilder 实例，支持复杂条件查询
//
template<typename Entity>
class BaseRepository {
    static_assert(std::is_base_of_v<ReflectiveEntity, Entity>,
                  "Entity must inherit from ReflectiveEntity");
    static_assert(std::is_default_constructible_v<Entity>,
                  "Entity must be default constructible");

public:
    // ========================================================================
    // 构造与析构
    // ========================================================================

    /// @brief 使用数据库驱动构造仓库
    /// @param driver 数据库驱动（多态，支持任意数据库）
    explicit BaseRepository(std::shared_ptr<IDatabaseDriver> driver)
        : m_driver(std::move(driver)) {}

    virtual ~BaseRepository() = default;

    BaseRepository(const BaseRepository&) = delete;
    BaseRepository& operator=(const BaseRepository&) = delete;

    // ========================================================================
    // 元数据访问
    // ========================================================================

    /// @brief 获取实体对应的表名
    [[nodiscard]] QString tableName() const { return EntityTraits<Entity>::tableName(); }

    /// @brief 获取实体的主键列名
    [[nodiscard]] QString primaryKey() const { return EntityTraits<Entity>::primaryKey(); }

    /// @brief 获取底层数据库驱动
    [[nodiscard]] std::shared_ptr<IDatabaseDriver> driver() const { return m_driver; }

    // ========================================================================
    // 插入 (INSERT)
    // ========================================================================

    /// @brief 插入单条记录
    /// @param entity 实体对象（自增主键会自动回填）
    /// @return 插入后的实体（含回填主键）
    Result<Entity> insert(Entity& entity) {
        auto columns = nonPrimaryColumns();
        if (columns.isEmpty()) {
            return Error(ErrorCode::InvalidArgument, "No non-primary columns to insert");
        }

        QStringList placeholders;
        std::vector<QVariant> values;
        for (const auto& col : columns) {
            placeholders << QStringLiteral("?");
            values.push_back(entity.getProperty(col));
        }

        QString sql = QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
            .arg(tableName())
            .arg(columns.join(QStringLiteral(", ")))
            .arg(placeholders.join(QStringLiteral(", ")));

        auto result = m_driver->executeUpdate(sql, values);
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }

        // 回填自增主键: 使用数据库无关的 lastInsertId()
        auto lastIdResult = m_driver->lastInsertId();
        if (lastIdResult.isOk()) {
            QVariant lastId = lastIdResult.unwrap();
            if (lastId.isValid() && !lastId.isNull()) {
                entity.setProperty(primaryKey(), lastId);
            }
        }

        return entity;
    }

    /// @brief 批量插入
    /// @param entities 实体列表
    /// @return 成功返回 Ok，失败返回错误
    Result<void> insertBatch(std::vector<Entity>& entities) {
        if (entities.empty()) return {};

        auto columns = nonPrimaryColumns();
        if (columns.isEmpty()) {
            return Error(ErrorCode::InvalidArgument, "No non-primary columns to insert");
        }

        m_driver->beginTransaction();

        QStringList placeholders;
        for (int i = 0; i < columns.size(); ++i) {
            placeholders << QStringLiteral("?");
        }

        QString sql = QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
            .arg(tableName())
            .arg(columns.join(QStringLiteral(", ")))
            .arg(placeholders.join(QStringLiteral(", ")));

        for (auto& entity : entities) {
            std::vector<QVariant> values;
            for (const auto& col : columns) {
                values.push_back(entity.getProperty(col));
            }

            auto result = m_driver->executeUpdate(sql, values);
            if (!result.isOk()) {
                m_driver->rollback();
                return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
            }

            // 回填自增主键 [v2.0.0]
            auto lastIdResult = m_driver->lastInsertId();
            if (lastIdResult.isOk()) {
                QVariant lastId = lastIdResult.unwrap();
                if (lastId.isValid() && !lastId.isNull()) {
                    entity.setProperty(primaryKey(), lastId);
                }
            }
        }

        auto commitResult = m_driver->commit();
        if (!commitResult.isOk()) {
            m_driver->rollback();
            return commitResult;
        }

        return {};
    }

    // ========================================================================
    // 删除 (DELETE)
    // ========================================================================

    /// @brief 按主键删除
    /// @param id 主键值
    /// @return 成功返回 Ok，失败返回错误
    Result<void> deleteById(const QVariant& id) {
        QString sql = QStringLiteral("DELETE FROM %1 WHERE %2 = ?")
            .arg(tableName(), primaryKey());
        auto result = m_driver->executeUpdate(sql, {id});
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }
        return {};
    }

    /// @brief 按条件删除
    /// @param whereClause WHERE 条件（不含 "WHERE" 关键字）
    /// @param params 条件参数
    /// @return 成功返回 Ok，失败返回错误
    Result<void> deleteByCondition(const QString& whereClause,
                                    const std::vector<QVariant>& params = {}) {
        QString sql = QStringLiteral("DELETE FROM %1 WHERE %2")
            .arg(tableName(), whereClause);
        auto result = m_driver->executeUpdate(sql, params);
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }
        return {};
    }

    // ========================================================================
    // 更新 (UPDATE)
    // ========================================================================

    /// @brief 按主键更新
    /// @param entity 实体对象（主键字段用于定位记录）
    /// @return 成功返回 Ok，失败返回错误
    Result<void> updateById(const Entity& entity) {
        auto columns = nonPrimaryColumns();
        QStringList setClauses;
        std::vector<QVariant> values;

        for (const auto& col : columns) {
            setClauses << QStringLiteral("%1 = ?").arg(col);
            values.push_back(entity.getProperty(col));
        }

        // 主键值作为 WHERE 条件参数
        values.push_back(entity.getProperty(primaryKey()));

        QString sql = QStringLiteral("UPDATE %1 SET %2 WHERE %3 = ?")
            .arg(tableName())
            .arg(setClauses.join(QStringLiteral(", ")))
            .arg(primaryKey());

        auto result = m_driver->executeUpdate(sql, values);
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }
        return {};
    }

    /// @brief 按条件更新
    /// @param entity 实体对象（包含要更新的字段值）
    /// @param whereClause WHERE 条件（不含 "WHERE" 关键字）
    /// @param params 条件参数
    Result<void> updateByCondition(const Entity& entity,
                                    const QString& whereClause,
                                    const std::vector<QVariant>& params = {}) {
        auto columns = nonPrimaryColumns();
        QStringList setClauses;
        std::vector<QVariant> values;

        for (const auto& col : columns) {
            setClauses << QStringLiteral("%1 = ?").arg(col);
            values.push_back(entity.getProperty(col));
        }

        // 追加 WHERE 参数
        values.insert(values.end(), params.begin(), params.end());

        QString sql = QStringLiteral("UPDATE %1 SET %2 WHERE %3")
            .arg(tableName())
            .arg(setClauses.join(QStringLiteral(", ")))
            .arg(whereClause);

        auto result = m_driver->executeUpdate(sql, values);
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }
        return {};
    }

    // ========================================================================
    // 查询 (SELECT)
    // ========================================================================

    /// @brief 按主键查询
    /// @param id 主键值
    /// @return 实体（若不存在则返回 Error）
    Result<Entity> selectById(const QVariant& id) {
        QString sql = QStringLiteral("SELECT * FROM %1 WHERE %2 = ?")
            .arg(tableName(), primaryKey());
        auto result = m_driver->executeQuery(sql, {id});
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }
        auto& rows = result.unwrap().rows;
        if (rows.empty()) {
            return Error(ErrorCode::NotFound,
                         "Entity not found in " + tableName().toStdString());
        }
        return rowToEntity(rows[0]);
    }

    /// @brief 查询全部
    /// @return 实体列表
    Result<std::vector<Entity>> selectList() {
        QString sql = QStringLiteral("SELECT * FROM %1").arg(tableName());
        return executeSelect(sql, {});
    }

    /// @brief 按条件查询
    /// @param whereClause WHERE 条件（不含 "WHERE" 关键字）
    /// @param params 条件参数
    /// @return 实体列表
    Result<std::vector<Entity>> selectByCondition(const QString& whereClause,
                                                   const std::vector<QVariant>& params = {}) {
        QString sql = QStringLiteral("SELECT * FROM %1 WHERE %2")
            .arg(tableName(), whereClause);
        return executeSelect(sql, params);
    }

    /// @brief 分页查询
    /// @param page 页码（从 1 开始）
    /// @param size 每页条数
    /// @return 实体列表
    Result<std::vector<Entity>> selectPage(int page, int size) {
        auto result = count();
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }

        int offset = (page - 1) * size;
        QString sql = QStringLiteral("SELECT * FROM %1 LIMIT ? OFFSET ?")
            .arg(tableName());
        return executeSelect(sql, {QVariant(size), QVariant(offset)});
    }

    /// @brief 统计总数
    /// @return 记录总数
    Result<int> count() {
        QString sql = QStringLiteral("SELECT COUNT(*) as cnt FROM %1").arg(tableName());
        auto result = m_driver->executeQuery(sql, {});
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }
        auto& rows = result.unwrap().rows;
        if (rows.empty()) return 0;
        auto it = rows[0].find(QStringLiteral("cnt"));
        if (it != rows[0].end()) {
            return it->second.toInt();
        }
        return 0;
    }

    /// @brief 检查记录是否存在
    /// @param id 主键值
    /// @return true=存在，false=不存在
    Result<bool> exists(const QVariant& id) {
        QString sql = QStringLiteral("SELECT 1 FROM %1 WHERE %2 = ? LIMIT 1")
            .arg(tableName(), primaryKey());
        auto result = m_driver->executeQuery(sql, {id});
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }
        return !result.unwrap().rows.empty();
    }

    // ========================================================================
    // 查询构建器
    // ========================================================================

    /// @brief 获取查询构建器，用于复杂条件查询
    /// @return QueryBuilder 实例
    ///
    /// @par 使用示例
    /// @code
    /// auto users = repo.queryBuilder()
    ///     .select({"id", "name", "email"})
    ///     .where("age", ">", 18)
    ///     .andWhere("status", "=", "active")
    ///     .orderBy("name", true)
    ///     .limit(10)
    ///     .build();
    /// auto result = repo.driver()->executeQuery(users.sql, users.params);
    /// @endcode
    [[nodiscard]] QueryBuilder queryBuilder() const {
        return QueryBuilder().from(tableName());
    }

protected:
    // ========================================================================
    // 内部辅助方法
    // ========================================================================

    /// @brief 执行 SELECT 查询并映射为实体列表
    Result<std::vector<Entity>> executeSelect(const QString& sql,
                                               const std::vector<QVariant>& params) {
        auto result = m_driver->executeQuery(sql, params);
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, result.unwrapErr().toStdString());
        }

        std::vector<Entity> entities;
        for (const auto& row : result.unwrap().rows) {
            entities.push_back(rowToEntity(row));
        }
        return entities;
    }

    /// @brief 将数据库行映射为实体对象
    Entity rowToEntity(const std::map<QString, QVariant>& row) const {
        Entity entity;
        for (const auto& [col, val] : row) {
            entity.setProperty(col, val);
        }
        return entity;
    }

    /// @brief 获取非主键的列名列表（首次计算后缓存）
    [[nodiscard]] QStringList nonPrimaryColumns() const {
        if (!m_nonPkColumnsCached) {
            Entity temp;
            QStringList names = temp.propertyNames();
            QString pk = primaryKey();
            names.removeAll(pk);
            m_nonPkColumnsCache = names;
            m_nonPkColumnsCached = true;
        }
        return m_nonPkColumnsCache;
    }

    std::shared_ptr<IDatabaseDriver> m_driver;
    mutable QStringList m_nonPkColumnsCache;   ///< 非主键列名缓存 [v2.0.0]
    mutable bool m_nonPkColumnsCached = false;  ///< 缓存是否有效 [v2.0.0]
};

} // namespace data
} // namespace sc

#endif // SOUL_DATA_BASE_REPOSITORY_H