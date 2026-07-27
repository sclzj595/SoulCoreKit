#ifndef SOUL_ORM_TYPED_QUERY_WRAPPER_H
#define SOUL_ORM_TYPED_QUERY_WRAPPER_H

#include <QString>
#include <QVariant>
#include <vector>
#include <functional>
#include <type_traits>
#include "soul/orm/column.h"
#include "soul/orm/query_wrapper.h"

namespace sc {
namespace orm {

// 前向声明,避免头文件传递依赖
class ISqlDialect;

/**
 * @brief 类型安全的查询构造器
 *
 * 基于 QueryWrapper 之上的类型安全封装，通过 Column 模板绑定实体字段，
 * 消除字符串硬编码，在编译期捕获字段拼写错误。
 *
 * @par 设计原则
 * - 组合优于继承：内部委托给 QueryWrapper，不修改 QueryWrapper
 * - 零开销抽象：模板在编译期展开，无运行时开销
 * - 兼容性：通过 unwrap() 可降级为 QueryWrapper，与现有 Repository 接口对接
 *
 * @par 使用示例
 * @code
 * TypedQueryWrapper<User> query;
 * query.eq(User::Username, "alice")
 *      .gt(User::Age, 18)
 *      .orderBy(User::CreateTime, false);
 *
 * auto repo = std::make_shared<ReadWriteRepository<User>>(driver);
 * auto result = repo->find(query.unwrap());
 * @endcode
 *
 * @tparam EntityT 实体类型
 *
 * @thread_safety Single-Threaded — 与 QueryWrapper 一致，单线程使用
 *
 * @see QueryWrapper, Column
 */
template<typename EntityT>
class TypedQueryWrapper {
public:
    using EntityType = EntityT;

    TypedQueryWrapper() = default;

    // ===== 比较条件 =====

    /**
     * @brief 等于条件: column = value
     */
    template<typename FieldType>
    TypedQueryWrapper& eq(const Column<EntityT, FieldType>& col, const FieldType& value) {
        m_inner.eq(col.name(), QVariant::fromValue(value));
        return *this;
    }

    /**
     * @brief 不等于条件: column <> value
     */
    template<typename FieldType>
    TypedQueryWrapper& ne(const Column<EntityT, FieldType>& col, const FieldType& value) {
        m_inner.ne(col.name(), QVariant::fromValue(value));
        return *this;
    }

    /**
     * @brief 大于条件: column > value
     */
    template<typename FieldType>
    TypedQueryWrapper& gt(const Column<EntityT, FieldType>& col, const FieldType& value) {
        m_inner.gt(col.name(), QVariant::fromValue(value));
        return *this;
    }

    /**
     * @brief 大于等于条件: column >= value
     */
    template<typename FieldType>
    TypedQueryWrapper& ge(const Column<EntityT, FieldType>& col, const FieldType& value) {
        m_inner.ge(col.name(), QVariant::fromValue(value));
        return *this;
    }

    /**
     * @brief 小于条件: column < value
     */
    template<typename FieldType>
    TypedQueryWrapper& lt(const Column<EntityT, FieldType>& col, const FieldType& value) {
        m_inner.lt(col.name(), QVariant::fromValue(value));
        return *this;
    }

    /**
     * @brief 小于等于条件: column <= value
     */
    template<typename FieldType>
    TypedQueryWrapper& le(const Column<EntityT, FieldType>& col, const FieldType& value) {
        m_inner.le(col.name(), QVariant::fromValue(value));
        return *this;
    }

    // ===== LIKE 条件 =====

    /**
     * @brief LIKE 条件: column LIKE value
     * @note value 需自行包含通配符(%)
     */
    TypedQueryWrapper& like(const Column<EntityT, QString>& col, const QString& value) {
        m_inner.like(col.name(), value);
        return *this;
    }

    /**
     * @brief NOT LIKE 条件: column NOT LIKE value
     */
    TypedQueryWrapper& notLike(const Column<EntityT, QString>& col, const QString& value) {
        m_inner.notLike(col.name(), value);
        return *this;
    }

    /**
     * @brief 左 LIKE: column LIKE %value
     */
    TypedQueryWrapper& likeLeft(const Column<EntityT, QString>& col, const QString& value) {
        m_inner.likeLeft(col.name(), value);
        return *this;
    }

    /**
     * @brief 右 LIKE: column LIKE value%
     */
    TypedQueryWrapper& likeRight(const Column<EntityT, QString>& col, const QString& value) {
        m_inner.likeRight(col.name(), value);
        return *this;
    }

    // ===== IN 条件 =====

    /**
     * @brief IN 条件: column IN (v1, v2, ...)
     */
    template<typename FieldType>
    TypedQueryWrapper& in(const Column<EntityT, FieldType>& col, const std::vector<FieldType>& values) {
        std::vector<QVariant> variants;
        variants.reserve(values.size());
        for (const auto& v : values) {
            variants.push_back(QVariant::fromValue(v));
        }
        m_inner.in(col.name(), variants);
        return *this;
    }

    /**
     * @brief NOT IN 条件: column NOT IN (v1, v2, ...)
     */
    template<typename FieldType>
    TypedQueryWrapper& notIn(const Column<EntityT, FieldType>& col, const std::vector<FieldType>& values) {
        std::vector<QVariant> variants;
        variants.reserve(values.size());
        for (const auto& v : values) {
            variants.push_back(QVariant::fromValue(v));
        }
        m_inner.notIn(col.name(), variants);
        return *this;
    }

    // ===== NULL 条件 =====

    /**
     * @brief IS NULL 条件
     */
    template<typename FieldType>
    TypedQueryWrapper& isNull(const Column<EntityT, FieldType>& col) {
        m_inner.isNull(col.name());
        return *this;
    }

    /**
     * @brief IS NOT NULL 条件
     */
    template<typename FieldType>
    TypedQueryWrapper& isNotNull(const Column<EntityT, FieldType>& col) {
        m_inner.isNotNull(col.name());
        return *this;
    }

    // ===== 逻辑组合 =====

    /**
     * @brief AND 嵌套组
     * @code
     * query.and_([](auto& q) {
     *     q.gt(User::Age, 18).lt(User::Age, 60);
     * });
     * @endcode
     */
    TypedQueryWrapper& and_(const std::function<void(TypedQueryWrapper&)>& func) {
        m_inner.and_([this, &func](QueryWrapper& inner) {
            TypedQueryWrapper typedInner;
            typedInner.m_inner = std::move(inner);
            func(typedInner);
            inner = std::move(typedInner.m_inner);
        });
        return *this;
    }

    /**
     * @brief OR 嵌套组
     */
    TypedQueryWrapper& or_(const std::function<void(TypedQueryWrapper&)>& func) {
        m_inner.or_([this, &func](QueryWrapper& inner) {
            TypedQueryWrapper typedInner;
            typedInner.m_inner = std::move(inner);
            func(typedInner);
            inner = std::move(typedInner.m_inner);
        });
        return *this;
    }

    // ===== 排序与分组 =====

    /**
     * @brief 排序
     * @param asc true=升序, false=降序
     */
    template<typename FieldType>
    TypedQueryWrapper& orderBy(const Column<EntityT, FieldType>& col, bool asc = true) {
        m_inner.orderBy(col.name(), asc);
        return *this;
    }

    /**
     * @brief 分组
     */
    template<typename FieldType>
    TypedQueryWrapper& groupBy(const Column<EntityT, FieldType>& col) {
        m_inner.groupBy(col.name());
        return *this;
    }

    // ===== 分页 =====

    /**
     * @brief 限制返回行数
     */
    TypedQueryWrapper& limit(int size) {
        m_inner.limit(size);
        return *this;
    }

    /**
     * @brief 偏移行数
     */
    TypedQueryWrapper& offset(int offset) {
        m_inner.offset(offset);
        return *this;
    }

    /**
     * @brief 允许全表 UPDATE/DELETE（默认禁止，防止误操作）
     */
    TypedQueryWrapper& allowFullTableOperation(bool allow = true) {
        m_inner.allowFullTableOperation(allow);
        return *this;
    }

    // ===== 降级与导出 =====

    /**
     * @brief 降级为底层 QueryWrapper
     *
     * 用于与现有 Repository 接口对接（Repository::find 接受 QueryWrapper）。
     */
    QueryWrapper& unwrap() { return m_inner; }

    /**
     * @brief 降级为 const QueryWrapper
     */
    [[nodiscard]] const QueryWrapper& unwrap() const { return m_inner; }

    /**
     * @brief 设置 SQL 方言
     */
    void setDialect(ISqlDialect* dialect) { m_inner.setDialect(dialect); }

    /**
     * @brief 获取 SQL 方言
     */
    [[nodiscard]] ISqlDialect* dialect() const { return m_inner.dialect(); }

private:
    QueryWrapper m_inner;
};

} // namespace orm
} // namespace sc

#endif // SOUL_ORM_TYPED_QUERY_WRAPPER_H
