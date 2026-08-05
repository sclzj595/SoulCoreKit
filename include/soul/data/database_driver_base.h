#ifndef SOUL_DATA_DATABASE_DRIVER_BASE_H
#define SOUL_DATA_DATABASE_DRIVER_BASE_H

// ============================================================================
// database_driver_base.h — 数据库驱动公共逻辑模板基类 [v1.9.2 新增]
// ============================================================================
//
// 设计目标: 提取 executeQuery/executeUpdate 的公共 SQL 执行逻辑到模板基类,
// 消除 SqliteDriver/MySqlDriver/PostgreSqlDriver 中的重复代码。
//
// 设计原则:
//   - CRTP 模式: 子类通过模板参数传递自身类型,实现编译期多态
//   - 钩子方法: 子类仅需实现 open() 和 getType(),其他操作由基类提供
//   - 零开销: 模板在编译期展开,无虚函数调用开销
//
// 用法:
//   class SqliteDriver : public DatabaseDriverBase<SqliteDriver> {
//   public:
//       Result<void> open(const ConnectionConfig& config) override { ... }
//       DatabaseType getType() const override { return DatabaseType::SQLite; }
//   };

#include <QString>
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QUuid>
#include <vector>
#include <memory>
#include "soul/data/database_driver.h"
#include "soul/core/result.h"
#include "soul/core/error.h"

namespace sc {
namespace data {

// ============================================================================
// DatabaseDriverBase<T> — 数据库驱动公共逻辑模板基类
// ============================================================================
//
// 封装了:
//   - executeQuery: prepare → bindValue → exec → 遍历结果集
//   - executeUpdate: prepare → bindValue → exec → 返回受影响行数
//   - 事务管理: beginTransaction / commit / rollback
//   - 连接管理: close / isConnected / getLastError / getConnectionId
//
// 子类职责:
//   - open(): 根据数据库类型打开连接(设置 driver name、参数等)
//   - getType(): 返回 DatabaseType 枚举值
//
// @tparam Derived 子类类型(CRTP)
template<typename Derived>
class DatabaseDriverBase : public IDatabaseDriver {
public:
    ~DatabaseDriverBase() override { (void)close(); }

    // ========================================================================
    // 连接管理
    // ========================================================================
    Result<void> close() override {
        if (m_db.isOpen()) {
            QString dbName = m_db.connectionName();
            m_db.close();
            QSqlDatabase::removeDatabase(dbName);
        }
        return {};
    }

    bool isConnected() const override { return m_db.isOpen(); }

    QString getLastError() const override { return m_db.lastError().text(); }

    QString getConnectionId() const override {
        return QString::fromStdString(m_connectionId);
    }

    // ========================================================================
    // SQL 执行 — 公共模板逻辑 [v1.9.2 提取]
    // ========================================================================

    /// @brief 执行查询并返回结果集
    /// @details 自动处理 prepare → bindValue → exec → 遍历结果集
    Result<QueryResult> executeQuery(const QString& sql,
                                      const std::vector<QVariant>& params = {}) override {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }

        QSqlQuery query(m_db);
        if (!query.prepare(sql)) {
            return Error(ErrorCode::QueryFailed, query.lastError().text());
        }

        bindParams(query, params);

        if (!query.exec()) {
            return Error(ErrorCode::QueryFailed, query.lastError().text());
        }

        QueryResult result;
        result.success = true;

        QSqlRecord record = query.record();
        while (query.next()) {
            std::map<QString, QVariant> row;
            for (int i = 0, cnt = static_cast<int>(record.count()); i < cnt; ++i) {
                row[record.fieldName(i)] = query.value(i);
            }
            result.rows.push_back(std::move(row));
        }

        return result;
    }

    /// @brief 执行更新并返回受影响行数
    /// @details 自动处理 prepare → bindValue → exec
    Result<int> executeUpdate(const QString& sql,
                               const std::vector<QVariant>& params = {}) override {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }

        QSqlQuery query(m_db);
        if (!query.prepare(sql)) {
            return Error(ErrorCode::QueryFailed, query.lastError().text());
        }

        bindParams(query, params);

        if (!query.exec()) {
            return Error(ErrorCode::QueryFailed, query.lastError().text());
        }

        m_lastInsertId = query.lastInsertId();
        return query.numRowsAffected();
    }

    // ========================================================================
    // 事务管理
    // ========================================================================
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

    /// @brief 获取最后一次 INSERT 操作的自增主键值 [v2.0.0]
    /// @details 使用 QSqlQuery::lastInsertId() 实现数据库无关的自增主键获取
    Result<QVariant> lastInsertId() override {
        return m_lastInsertId;
    }

protected:
    /// @brief 生成唯一连接 ID
    void generateConnectionId() {
        m_connectionId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    }

    QSqlDatabase m_db;
    std::string m_connectionId;
    bool m_inTransaction = false;
    QVariant m_lastInsertId;  ///< 最后一次 INSERT 的自增主键值 [v2.0.0]

private:
    /// @brief 绑定参数到查询
    static void bindParams(QSqlQuery& query, const std::vector<QVariant>& params) {
        for (size_t i = 0; i < params.size(); ++i) {
            query.bindValue(static_cast<int>(i), params[i]);
        }
    }
};

} // namespace data
} // namespace sc

#endif // SOUL_DATA_DATABASE_DRIVER_BASE_H