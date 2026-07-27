#ifndef SOUL_ORM_COLUMN_H
#define SOUL_ORM_COLUMN_H

#include <QString>
#include <utility>

namespace sc {
namespace orm {

/**
 * @brief 类型安全的列引用
 *
 * 将实体字段名与字段类型在编译期绑定，消除字符串硬编码导致的 SQL 注入和拼写错误。
 * 仅作为元数据载体，不持有实体实例，零运行时开销。
 *
 * @par 使用方式
 * @code
 * class User : public Entity<User> {
 * public:
 *     static inline const Column<User, QString> Id{"id"};
 *     static inline const Column<User, QString> Username{"username"};
 *     static inline const Column<User, int> Age{"age"};
 * };
 *
 * TypedQueryWrapper<User> query;
 * query.eq(User::Username, "alice");
 * @endcode
 *
 * @tparam EntityT    实体类型
 * @tparam FieldType  字段类型
 *
 * @thread_safety Immutable — 静态常量,线程安全
 */
template<typename EntityT, typename FieldType>
class Column {
public:
    using EntityType = EntityT;
    using ValueType  = FieldType;

    /**
     * @brief 构造列引用
     * @param name 列名(数据库列名或属性名,由使用方约定)
     */
    explicit Column(QString name) noexcept
        : m_name(std::move(name)) {}

    /**
     * @brief 获取列名
     */
    [[nodiscard]] const QString& name() const noexcept { return m_name; }

private:
    QString m_name;
};

} // namespace orm
} // namespace sc

#endif // SOUL_ORM_COLUMN_H
