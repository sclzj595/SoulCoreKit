#ifndef SOUL_DATA_REPOSITORY_FACTORY_H
#define SOUL_DATA_REPOSITORY_FACTORY_H

// ============================================================================
// repository_factory.h — Repository 自动实现代理(对标 MyBatis-Plus BaseMapper)
// ============================================================================
//
// 设计目标: 根据实体反射元数据自动生成 CRUD 实现,消除手写 Repository 子类样板代码。
// 对标 MyBatis-Plus 的 BaseMapper<T> 自动 CRUD 能力。
//
// 设计原则:
//   - 零样板: 一个工厂调用生成完整 CRUD 实现
//   - 反射驱动: 利用 ORM 模块的 ReflectionTable 自动生成 SQL
//   - 方言抽象: 通过 ISqlDialect 支持多数据库(SQLite/MySQL/PostgreSQL)
//   - 类型安全: 编译期绑定实体类型,运行时通过反射表操作字段
//
// 用法:
//   // 1. 定义实体
//   class User : public Entity<User> {
//   public:
//       SC_TABLE(User, "users")
//       QString name;
//       int age;
//       SC_DEFINE_REFLECTION(User,
//           SC_REF_FIELD("id",   &User::id)
//           SC_REF_FIELD("name", &User::name)
//           SC_REF_FIELD("age",  &User::age)
//       )
//   };
//
//   // 2. 通过工厂创建 Repository(自动生成 CRUD)
//   auto repo = RepositoryFactory<User>::create(dialect, connection, "users");
//   auto result = repo->findById("user-123");
//   auto all = repo->findAll();
//
// @thread_safety create() 返回的 Repository 实例是线程安全的(读操作用 shared_mutex,写操作排他)
// [v1.9.2] 读操作(findById/findAll/existsById/count)使用 shared_mutex 提升并发读性能

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include <cctype>

#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QVariant>

#include "soul/data/repository.h"
#include "soul/orm/reflection.h"
#include "soul/orm/sql_dialect.h"
#include "soul/core/result.h"
#include "soul/core/error.h"

namespace sc {
namespace data {

// ============================================================================
// AutoRepository — 自动生成的 CRUD Repository 实现 [v1.9.1 新增]
// ============================================================================
//
// 通过反射表自动生成 SQL 语句,无需手写任何 Repository 代码。
//
// @tparam T     实体类型(需通过 SC_DEFINE_REFLECTION 声明反射表)
// @tparam Id    实体 ID 类型
template<typename T, typename Id = QString>
class AutoRepository : public IRepository<T, Id> {
public:
    /// @param dialect   SQL 方言(用于生成 SQL 语法)
    /// @param db        QSqlDatabase 连接(调用方管理生命周期)
    /// @param tableName 表名
    explicit AutoRepository(
        std::shared_ptr<orm::ISqlDialect> dialect,
        QSqlDatabase db,
        std::string tableName)
        : m_dialect(std::move(dialect))
        , m_db(std::move(db))
        , m_tableName(std::move(tableName))
    {}

    Result<T> findById(const Id& id) override {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        QSqlQuery query(m_db);
        QString sql = QStringLiteral("SELECT * FROM %1 WHERE %2 = ?")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)),
                 m_dialect->escapeIdentifier(QStringLiteral("id")));
        query.prepare(sql);
        query.addBindValue(QVariant::fromValue(id));

        if (!query.exec() || !query.next()) {
            return Error(ErrorCode::NotFound,
                         "Entity not found: " + m_tableName + " id=" +
                         QVariant::fromValue(id).toString().toStdString());
        }
        return mapRow(query);
    }

    Result<std::vector<T>> findAll() override {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        QSqlQuery query(m_db);
        QString sql = QStringLiteral("SELECT * FROM %1")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)));
        query.prepare(sql);

        if (!query.exec()) {
            return Error(ErrorCode::InternalError,
                         "Failed to query: " + query.lastError().text().toStdString());
        }
        std::vector<T> result;
        while (query.next()) {
            result.push_back(mapRow(query));
        }
        return result;
    }

    Result<T> save(const T& entity) override {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        const auto& fields = T::_sc_reflection().fieldNames();
        if (fields.empty()) {
            return Error(ErrorCode::InvalidArgument, "Entity has no reflection fields");
        }

        QStringList columns, placeholders;
        QVariantList values;
        for (const auto& name : fields) {
            columns << m_dialect->escapeIdentifier(name);
            placeholders << QStringLiteral("?");
            values << T::_sc_reflection().get(entity, name);
        }

        QString sql = QStringLiteral("INSERT OR REPLACE INTO %1 (%2) VALUES (%3)")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)),
                 columns.join(QStringLiteral(", ")),
                 placeholders.join(QStringLiteral(", ")));

        QSqlQuery query(m_db);
        query.prepare(sql);
        for (const auto& v : values) {
            query.addBindValue(v);
        }

        if (!query.exec()) {
            return Error(ErrorCode::InternalError,
                         "Failed to save: " + query.lastError().text().toStdString());
        }
        return entity;
    }

    Result<void> removeById(const Id& id) override {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        QSqlQuery query(m_db);
        QString sql = QStringLiteral("DELETE FROM %1 WHERE %2 = ?")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)),
                 m_dialect->escapeIdentifier(QStringLiteral("id")));
        query.prepare(sql);
        query.addBindValue(QVariant::fromValue(id));

        if (!query.exec()) {
            return Error(ErrorCode::InternalError,
                         "Failed to delete: " + query.lastError().text().toStdString());
        }
        return {};
    }

    Result<bool> existsById(const Id& id) override {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        QSqlQuery query(m_db);
        QString sql = QStringLiteral("SELECT COUNT(*) FROM %1 WHERE %2 = ?")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)),
                 m_dialect->escapeIdentifier(QStringLiteral("id")));
        query.prepare(sql);
        query.addBindValue(QVariant::fromValue(id));

        if (!query.exec() || !query.next()) {
            return false;
        }
        return query.value(0).toInt() > 0;
    }

    // ========================================================================
    // 扩展方法 [v1.9.1 新增]
    // ========================================================================

    /// @brief 按字段名查询单个实体
    /// @param fieldName 字段名(需在反射表中注册)
    /// @param value     字段值
    /// @return Result<T>
    Result<T> findByField(const QString& fieldName, const QVariant& value) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        QSqlQuery query(m_db);
        QString sql = QStringLiteral("SELECT * FROM %1 WHERE %2 = ?")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)),
                 m_dialect->escapeIdentifier(fieldName));
        query.prepare(sql);
        query.addBindValue(value);

        if (!query.exec() || !query.next()) {
            return Error(ErrorCode::NotFound,
                         "Entity not found: " + m_tableName + " " +
                         fieldName.toStdString() + "=" + value.toString().toStdString());
        }
        return mapRow(query);
    }

    /// @brief 按字段名查询所有匹配实体
    /// @param fieldName 字段名
    /// @param value     字段值
    /// @return Result<std::vector<T>>
    Result<std::vector<T>> findAllByField(const QString& fieldName, const QVariant& value) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        QSqlQuery query(m_db);
        QString sql = QStringLiteral("SELECT * FROM %1 WHERE %2 = ?")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)),
                 m_dialect->escapeIdentifier(fieldName));
        query.prepare(sql);
        query.addBindValue(value);

        if (!query.exec()) {
            return Error(ErrorCode::InternalError,
                         "Failed to query: " + query.lastError().text().toStdString());
        }
        std::vector<T> result;
        while (query.next()) {
            result.push_back(mapRow(query));
        }
        return result;
    }

    /// @brief 更新实体(基于 id 字段)
    /// @param entity 实体(需包含 id 值)
    /// @return Result<T>
    Result<T> update(const T& entity) {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        const auto& fields = T::_sc_reflection().fieldNames();
        if (fields.empty()) {
            return Error(ErrorCode::InvalidArgument, "Entity has no reflection fields");
        }

        // 获取 id 值
        QVariant idValue = T::_sc_reflection().get(entity, QStringLiteral("id"));
        if (!idValue.isValid()) {
            return Error(ErrorCode::InvalidArgument, "Entity has no valid id field");
        }

        QStringList setClauses;
        QVariantList values;
        for (const auto& name : fields) {
            if (name == QStringLiteral("id")) continue; // 跳过 id 字段
            setClauses << QStringLiteral("%1 = ?").arg(m_dialect->escapeIdentifier(name));
            values << T::_sc_reflection().get(entity, name);
        }

        if (setClauses.isEmpty()) {
            return Error(ErrorCode::InvalidArgument, "No fields to update");
        }

        QString sql = QStringLiteral("UPDATE %1 SET %2 WHERE %3 = ?")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)),
                 setClauses.join(QStringLiteral(", ")),
                 m_dialect->escapeIdentifier(QStringLiteral("id")));
        values << idValue;

        QSqlQuery query(m_db);
        query.prepare(sql);
        for (const auto& v : values) {
            query.addBindValue(v);
        }

        if (!query.exec()) {
            return Error(ErrorCode::InternalError,
                         "Failed to update: " + query.lastError().text().toStdString());
        }
        return entity;
    }

    /// @brief 分页查询
    /// @param page     页码(从 1 开始)
    /// @param pageSize 每页大小
    /// @param orderBy  排序字段(可选,为空则不排序)
    /// @return Result<std::vector<T>>
    Result<std::vector<T>> pageQuery(int page, int pageSize,
                                      const QString& orderBy = QString()) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        int offset = (page - 1) * pageSize;

        QString sql = QStringLiteral("SELECT * FROM %1")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)));

        if (!orderBy.isEmpty()) {
            sql += QStringLiteral(" ORDER BY %1").arg(m_dialect->escapeIdentifier(orderBy));
        }

        // 使用方言的 LIMIT/OFFSET 语法
        QString limitClause = m_dialect->buildLimitOffset(pageSize, offset);
        if (!limitClause.isEmpty()) {
            sql += QStringLiteral(" ") + limitClause;
        }

        QSqlQuery query(m_db);
        query.prepare(sql);

        if (!query.exec()) {
            return Error(ErrorCode::InternalError,
                         "Failed to query: " + query.lastError().text().toStdString());
        }
        std::vector<T> result;
        while (query.next()) {
            result.push_back(mapRow(query));
        }
        return result;
    }

    /// @brief 统计总数
    /// @return Result<int>
    Result<int> count() {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        QSqlQuery query(m_db);
        QString sql = QStringLiteral("SELECT COUNT(*) FROM %1")
            .arg(m_dialect->escapeIdentifier(QString::fromStdString(m_tableName)));
        query.prepare(sql);

        if (!query.exec() || !query.next()) {
            return Error(ErrorCode::InternalError,
                         "Failed to count: " + query.lastError().text().toStdString());
        }
        return query.value(0).toInt();
    }

    /// @brief 获取表名
    const std::string& tableName() const { return m_tableName; }

private:
    /// @brief 将 QSqlQuery 当前行映射为实体 T
    T mapRow(const QSqlQuery& query) const {
        T entity{};
        QSqlRecord record = query.record();
        for (int i = 0; i < record.count(); ++i) {
            QString fieldName = record.fieldName(i);
            QVariant value = query.value(i);
            T::_sc_reflection().set(entity, fieldName, value);
        }
        return entity;
    }

    std::shared_ptr<orm::ISqlDialect> m_dialect;
    QSqlDatabase m_db;
    std::string m_tableName;
    mutable std::shared_mutex m_mutex;  ///< [v1.9.2] shared_mutex 优化读并发
};

// ============================================================================
// RepositoryFactory — Repository 工厂(对标 MyBatis-Plus BaseMapper) [v1.9.1 新增]
// ============================================================================
//
// 根据实体类型自动创建 Repository 实例,无需手写子类。
//
// 用法:
//   auto repo = RepositoryFactory<User>::create(dialect, connection, "users");
//   auto result = repo->findById("user-123");
//
// @tparam T 实体类型(需通过 SC_DEFINE_REFLECTION 声明反射表)
template<typename T, typename Id = QString>
struct RepositoryFactory {
    /// @brief 创建自动生成的 Repository 实例
    /// @param dialect   SQL 方言
    /// @param db        QSqlDatabase 连接
    /// @param tableName 表名(默认使用类型名推断)
    /// @return std::unique_ptr<IRepository<T, Id>>
    static std::unique_ptr<IRepository<T, Id>> create(
        std::shared_ptr<orm::ISqlDialect> dialect,
        QSqlDatabase db,
        const std::string& tableName = "")
    {
        std::string tbl = tableName.empty() ? inferTableName() : tableName;
        return std::make_unique<AutoRepository<T, Id>>(
            std::move(dialect), std::move(db), std::move(tbl));
    }

private:
    /// @brief 从类型名推断表名(简单规则: 类名转小写)
    static std::string inferTableName() {
        std::string name = typeid(T).name();
        // 去掉可能的命名空间前缀
        auto pos = name.find_last_of("::");
        if (pos != std::string::npos) {
            name = name.substr(pos + 1);
        }
        // 转小写
        for (auto& c : name) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return name;
    }
};

} // namespace data
} // namespace sc

#endif // SOUL_DATA_REPOSITORY_FACTORY_H