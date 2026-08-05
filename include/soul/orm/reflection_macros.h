#ifndef SOUL_ORM_REFLECTION_MACROS_H
#define SOUL_ORM_REFLECTION_MACROS_H

#include "soul/orm/reflection.h"

/**
 * @file reflection_macros.h
 * @brief 实体反射宏 - 消除 getPropertyImpl/setPropertyImpl 样板代码
 *
 * @par 设计目标
 * - 类型安全：通过成员指针在编译期绑定字段类型
 * - 零样板：一行宏调用替代手写 getPropertyImpl/setPropertyImpl
 * - 非侵入式：不修改 Entity 基类，与既有 SC_FIELD 用法并存
 * - 静态初始化：反射表只构造一次，线程安全（C++11 magic statics）
 *
 * @par 使用示例
 * @code
 * class User : public Entity<User> {
 * public:
 *     SC_ENTITY_TABLE(User, "user")
 *
 *     QString username;
 *     QString email;
 *     int     age;
 *
 *     // 一行宏声明反射表 + 自动生成 getPropertyImpl/setPropertyImpl
 *     SC_DEFINE_REFLECTION(User,
 *         SC_REF_FIELD("username", &User::username)
 *         SC_REF_FIELD("email",    &User::email)
 *         SC_REF_FIELD("age",      &User::age)
 *     )
 * };
 * @endcode
 *
 * @note 宏生成的 getPropertyImpl/setPropertyImpl 不带 override，
 *       因为 Entity<Derived> 基类通过 CRTP 调用（非虚函数）。
 *       这与既有手写实现保持一致。
 */

/**
 * @brief 单个反射字段声明（在 SC_DEFINE_REFLECTION 内使用）
 *
 * 展开为 `.field(name, ptr)` 链式调用片段
 */
#define SC_REF_FIELD(name, ptr) .field(name, ptr)

/**
 * @brief 定义实体反射表并生成 getPropertyImpl/setPropertyImpl
 *
 * @param ClassName 实体类名
 * @param Fields    字段列表，每项用 SC_REF_FIELD 声明
 *
 * @par 生成的代码
 * 1. 静态方法 `_sc_reflection()` 返回 ReflectionTable 单例
 * 2. `getPropertyImpl(name)` 委托给反射表
 * 3. `setPropertyImpl(name, value)` 委托给反射表
 *
 * @note 使用 C++11 magic statics 保证线程安全初始化
 */
#define SC_DEFINE_REFLECTION(ClassName, Fields) \
    static const ::sc::orm::reflection::ReflectionTable<ClassName>& _sc_reflection() { \
        static const auto table = ::sc::orm::reflection::ReflectionTable<ClassName>() \
            Fields; \
        return table; \
    } \
    QVariant getPropertyImpl(const QString& _sc_name) const { \
        return _sc_reflection().get(*this, _sc_name); \
    } \
    void setPropertyImpl(const QString& _sc_name, const QVariant& _sc_value) { \
        _sc_reflection().set(*this, _sc_name, _sc_value); \
    }

#endif // SOUL_ORM_REFLECTION_MACROS_H
