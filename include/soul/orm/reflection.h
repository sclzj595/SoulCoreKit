#ifndef SOUL_ORM_REFLECTION_H
#define SOUL_ORM_REFLECTION_H

#include <QString>
#include <QVariant>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace sc {
namespace orm {
namespace reflection {

/**
 * @brief 实体反射表
 *
 * 通过成员指针注册实体字段，提供统一的 get/set 接口，
 * 消除手写 getPropertyImpl/setPropertyImpl 的样板代码。
 *
 * @par 设计原则
 * - 类型安全：成员指针在编译期绑定字段类型
 * - Type Erasure：通过 std::function 隐藏字段类型差异
 * - 静态初始化：反射表只构造一次，O(log N) 查找
 * - 非侵入式：不修改 Entity 基类，向后兼容
 *
 * @par 使用示例
 * @code
 * class User : public Entity<User> {
 * public:
 *     QString username;
 *     int age;
 *
 *     static const ReflectionTable<User>& reflection() {
 *         static const auto table = ReflectionTable<User>()
 *             .field("username", &User::username)
 *             .field("age", &User::age);
 *         return table;
 *     }
 *
 *     QVariant getPropertyImpl(const QString& name) const override {
 *         return reflection().get(*this, name);
 *     }
 *     void setPropertyImpl(const QString& name, const QVariant& value) override {
 *         reflection().set(*this, name, value);
 *     }
 * };
 * @endcode
 *
 * @tparam EntityType 实体类型
 *
 * @thread_safety Thread-Safe — 静态只读表,构造后可并发读
 */
template<typename EntityType>
class ReflectionTable {
public:
    /**
     * @brief 注册字段（链式调用）
     * @tparam FieldType 字段类型（自动推导）
     * @param name 字段名（与 getProperty/setProperty 使用的 key 一致）
     * @param ptr  成员指针
     * @return *this（支持链式调用）
     */
    template<typename FieldType>
    ReflectionTable& field(QString name, FieldType EntityType::* ptr) {
        m_getters[name] = [ptr](const EntityType& e) -> QVariant {
            return QVariant::fromValue(e.*ptr);
        };
        m_setters[name] = [ptr](EntityType& e, const QVariant& v) {
            e.*ptr = v.value<FieldType>();
        };
        m_names.push_back(name);
        return *this;
    }

    /**
     * @brief 获取字段值
     * @param entity 实体实例
     * @param name   字段名
     * @return 字段值；若字段不存在返回无效 QVariant
     */
    [[nodiscard]] QVariant get(const EntityType& entity, const QString& name) const {
        auto it = m_getters.find(name);
        if (it != m_getters.end()) {
            return it->second(entity);
        }
        return QVariant();
    }

    /**
     * @brief 设置字段值
     * @param entity 实体实例
     * @param name   字段名
     * @param value  字段值
     * @return true=设置成功; false=字段不存在
     */
    bool set(EntityType& entity, const QString& name, const QVariant& value) const {
        auto it = m_setters.find(name);
        if (it != m_setters.end()) {
            it->second(entity, value);
            return true;
        }
        return false;
    }

    /**
     * @brief 检查字段是否存在
     */
    [[nodiscard]] bool contains(const QString& name) const {
        return m_getters.find(name) != m_getters.end();
    }

    /**
     * @brief 获取所有已注册字段名
     */
    [[nodiscard]] const std::vector<QString>& fieldNames() const noexcept {
        return m_names;
    }

private:
    using Getter = std::function<QVariant(const EntityType&)>;
    using Setter = std::function<void(EntityType&, const QVariant&)>;

    std::map<QString, Getter> m_getters;
    std::map<QString, Setter> m_setters;
    std::vector<QString>      m_names;
};

} // namespace reflection
} // namespace orm
} // namespace sc

#endif // SOUL_ORM_REFLECTION_H
