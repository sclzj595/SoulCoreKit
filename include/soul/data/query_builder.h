#ifndef SOUL_DATA_QUERY_BUILDER_H
#define SOUL_DATA_QUERY_BUILDER_H

// ============================================================================
// query_builder.h — 类型安全的查询构建器 [v2.0.0 新增]
// ============================================================================
//
// 对标 MyBatis-Plus 的 QueryWrapper，提供链式 SQL 构建能力。
// 使用参数占位符 `?` 方式，防止 SQL 注入。
//
// 核心设计:
//   - QueryBuilder 类: 链式构建 SQL 查询
//   - 方法: select(columns), from(table), where(column, op, value),
//           andWhere(...), orWhere(...), orderBy(col, asc),
//           limit(n), offset(n)
//   - build() 返回 SQL 字符串 + 参数绑定列表
//   - 参数使用占位符 `?` 方式，防止 SQL 注入
//
// 用法:
//   QueryBuilder qb;
//   auto [sql, params] = qb.select({"id", "name", "email"})
//       .from("users")
//       .where("age", ">", 18)
//       .andWhere("status", "=", "active")
//       .orderBy("id", true)
//       .limit(10)
//       .build();
//   // sql = "SELECT id, name, email FROM users WHERE age > ? AND status = ? ORDER BY id ASC LIMIT 10"
//   // params = {18, "active"}

#include <QString>
#include <QStringList>
#include <QVariant>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace sc {
namespace data {

// ============================================================================
// QueryResult — 构建结果（SQL + 参数绑定列表）
// ============================================================================
///
/// @brief 查询构建结果，包含 SQL 字符串和参数绑定列表
struct QueryBuildResult {
    QString sql;                           ///< 完整 SQL 字符串（含 ? 占位符）
    std::vector<QVariant> params;          ///< 参数绑定列表（与 ? 一一对应）
};

// ============================================================================
// QueryBuilder — 链式查询构建器
// ============================================================================
///
/// @brief 类型安全的 SQL 查询构建器
///
/// 通过链式调用构建 SELECT 查询，自动将条件值收集为参数绑定列表，
/// 避免 SQL 注入风险。
///
/// @par 支持的子句
///   - SELECT: 指定查询列
///   - FROM:   指定数据表
///   - WHERE / AND / OR: 条件过滤
///   - ORDER BY: 排序
///   - LIMIT / OFFSET: 分页
///
/// @par 使用示例
/// @code
/// QueryBuilder qb;
/// auto [sql, params] = qb.select({"id", "name", "email"})
///     .from("users")
///     .where("age", ">", 18)
///     .andWhere("status", "=", QString("active"))
///     .orderBy("id", true)
///     .limit(10)
///     .build();
/// @endcode
///
/// @thread_safety 非线程安全 — 单线程使用
class QueryBuilder {
public:
    QueryBuilder() = default;
    ~QueryBuilder() = default;

    // ========================================================================
    // SELECT 子句
    // ========================================================================

    /// @brief 指定查询列
    /// @param columns 列名列表
    /// @return *this（链式调用）
    QueryBuilder& select(const QStringList& columns) {
        m_columns = columns;
        return *this;
    }

    /// @brief 指定查询列（initializer_list 版本）
    /// @param columns 列名列表
    /// @return *this（链式调用）
    QueryBuilder& select(std::initializer_list<QString> columns) {
        m_columns = QStringList(columns.begin(), columns.end());
        return *this;
    }

    // ========================================================================
    // FROM 子句
    // ========================================================================

    /// @brief 指定数据表
    /// @param table 表名
    /// @return *this（链式调用）
    QueryBuilder& from(const QString& table) {
        m_table = table;
        return *this;
    }

    // ========================================================================
    // WHERE 子句
    // ========================================================================

    /// @brief 添加 WHERE 条件（第一个条件）
    /// @param column 列名
    /// @param op 操作符（=, >, <, >=, <=, !=, LIKE, IN 等）
    /// @param value 绑定值
    /// @return *this（链式调用）
    QueryBuilder& where(const QString& column, const QString& op, const QVariant& value) {
        m_whereClauses.clear();
        m_whereParams.clear();
        addCondition("WHERE", column, op, value);
        return *this;
    }

    /// @brief 添加 AND 条件
    /// @param column 列名
    /// @param op 操作符
    /// @param value 绑定值
    /// @return *this（链式调用）
    QueryBuilder& andWhere(const QString& column, const QString& op, const QVariant& value) {
        addCondition(m_whereClauses.empty() ? "WHERE" : "AND", column, op, value);
        return *this;
    }

    /// @brief 添加 OR 条件
    /// @param column 列名
    /// @param op 操作符
    /// @param value 绑定值
    /// @return *this（链式调用）
    QueryBuilder& orWhere(const QString& column, const QString& op, const QVariant& value) {
        addCondition(m_whereClauses.empty() ? "WHERE" : "OR", column, op, value);
        return *this;
    }

    // ========================================================================
    // ORDER BY 子句
    // ========================================================================

    /// @brief 添加排序
    /// @param column 排序列名
    /// @param ascending 是否升序（true=ASC, false=DESC）
    /// @return *this（链式调用）
    QueryBuilder& orderBy(const QString& column, bool ascending = true) {
        m_orderBy = column;
        m_orderAsc = ascending;
        return *this;
    }

    // ========================================================================
    // LIMIT / OFFSET 子句
    // ========================================================================

    /// @brief 限制返回行数
    /// @param n 最大行数
    /// @return *this（链式调用）
    QueryBuilder& limit(int n) {
        m_limit = n;
        return *this;
    }

    /// @brief 偏移量
    /// @param n 跳过行数
    /// @return *this（链式调用）
    QueryBuilder& offset(int n) {
        m_offset = n;
        return *this;
    }

    // ========================================================================
    // build() — 构建最终 SQL
    // ========================================================================

    /// @brief 构建 SQL 和参数绑定列表
    /// @return QueryBuildResult{ sql, params }
    /// @note 若 FROM 表未设置，返回空 SQL（调用方需检查）
    [[nodiscard]] QueryBuildResult build() const {
        // 校验 FROM 子句 [v2.0.0]
        if (m_table.isEmpty()) {
            return {QString(), {}};
        }

        QString sql;

        // SELECT 子句
        sql += QStringLiteral("SELECT ");
        if (m_columns.isEmpty()) {
            sql += QStringLiteral("*");
        } else {
            sql += m_columns.join(QStringLiteral(", "));
        }

        // FROM 子句
        sql += QStringLiteral(" FROM ") + m_table;

        // WHERE 子句
        if (!m_whereClauses.isEmpty()) {
            sql += QStringLiteral(" ");
            sql += m_whereClauses.join(QStringLiteral(" "));
        }

        // ORDER BY 子句
        if (!m_orderBy.isEmpty()) {
            sql += QStringLiteral(" ORDER BY ") + m_orderBy;
            sql += m_orderAsc ? QStringLiteral(" ASC") : QStringLiteral(" DESC");
        }

        // LIMIT 子句(使用 ? 占位符参数化)
        std::vector<QVariant> params = m_whereParams;
        if (m_limit >= 0) {
            sql += QStringLiteral(" LIMIT ?");
            params.push_back(QVariant(m_limit));
        }

        // OFFSET 子句(使用 ? 占位符参数化)
        if (m_offset >= 0) {
            sql += QStringLiteral(" OFFSET ?");
            params.push_back(QVariant(m_offset));
        }

        return {sql, params};
    }

    /// @brief 重置构建器状态
    void reset() {
        m_columns.clear();
        m_table.clear();
        m_whereClauses.clear();
        m_whereParams.clear();
        m_orderBy.clear();
        m_orderAsc = true;
        m_limit = -1;
        m_offset = -1;
    }

private:
    /// @brief 添加条件子句
    void addCondition(const QString& prefix, const QString& column,
                      const QString& op, const QVariant& value) {
        m_whereClauses.append(
            QStringLiteral("%1 %2 %3 ?").arg(prefix, column, op));
        m_whereParams.push_back(value);
    }

    QStringList m_columns;
    QString m_table;
    QStringList m_whereClauses;
    std::vector<QVariant> m_whereParams;
    QString m_orderBy;
    bool m_orderAsc = true;
    int m_limit = -1;
    int m_offset = -1;
};

} // namespace data
} // namespace sc

#endif // SOUL_DATA_QUERY_BUILDER_H