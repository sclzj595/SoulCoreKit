#ifndef SOUL_DATA_ORM_REFLECTION_H
#define SOUL_DATA_ORM_REFLECTION_H

// ============================================================================
// orm_reflection.h — ORM 反射自动化 [v2.0.0 新增]
// ============================================================================
//
// 对标 MyBatis-Plus 的 BaseMapper，提供手动属性注册的反射机制。
// 不依赖宏自动展开，使用显式注册方式，类型安全且易于调试。
//
// 核心设计:
//   - EntityProperty 结构体: 属性名(name)、getter 函数、setter 函数
//   - ReflectiveEntity 基类: 存储属性映射表，提供 getProperty(name)/setProperty(name, value)
//   - SC_PROPERTY(ClassName, propName, getter, setter) 宏: 在构造函数中注册属性
//
// 支持的属性类型: int, double, QString, std::string, bool
//
// 用法:
//   class User : public ReflectiveEntity {
//   public:
//       User() {
//           SC_PROPERTY(User, id, getId, setId);
//           SC_PROPERTY(User, name, getName, setName);
//       }
//       int getId() const { return m_id; }
//       void setId(int v) { m_id = v; }
//       QString getName() const { return m_name; }
//       void setName(const QString& v) { m_name = v; }
//   private:
//       int m_id = 0;
//       QString m_name;
//   };

#include <QString>
#include <QVariant>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace sc {
namespace data {

// ============================================================================
// EntityProperty — 单个实体属性描述
// ============================================================================
///
/// @brief 描述实体的一个属性，包含名称、getter 和 setter
///
/// getter/setter 使用 std::function 实现类型擦除，支持不同属性类型。
///
/// @tparam EntityType 实体类型
struct EntityPropertyBase {
    QString name;

    /// @brief 从实体获取属性值（返回 QVariant）
    virtual QVariant getValue(const void* entity) const = 0;

    /// @brief 设置实体属性值
    virtual void setValue(void* entity, const QVariant& value) const = 0;

    virtual ~EntityPropertyBase() = default;

protected:
    explicit EntityPropertyBase(QString n) : name(std::move(n)) {}
};

/// @brief 具体类型的属性描述（模板化实现）
/// @tparam EntityType 实体类型
/// @tparam ValueType 属性值类型
template<typename EntityType, typename ValueType>
struct EntityProperty : public EntityPropertyBase {
    using Getter = std::function<ValueType(const EntityType&)>;
    using Setter = std::function<void(EntityType&, const ValueType&)>;

    Getter getter;
    Setter setter;

    EntityProperty(QString n, Getter g, Setter s)
        : EntityPropertyBase(std::move(n))
        , getter(std::move(g))
        , setter(std::move(s)) {}

    QVariant getValue(const void* entity) const override {
        const auto& e = *static_cast<const EntityType*>(entity);
        return QVariant::fromValue(getter(e));
    }

    void setValue(void* entity, const QVariant& value) const override {
        auto& e = *static_cast<EntityType*>(entity);
        if constexpr (std::is_same_v<ValueType, QString>) {
            setter(e, value.toString());
        } else if constexpr (std::is_same_v<ValueType, std::string>) {
            setter(e, value.toString().toStdString());
        } else if constexpr (std::is_same_v<ValueType, int>) {
            setter(e, value.toInt());
        } else if constexpr (std::is_same_v<ValueType, double>) {
            setter(e, value.toDouble());
        } else if constexpr (std::is_same_v<ValueType, bool>) {
            setter(e, value.toBool());
        } else {
            setter(e, value.value<ValueType>());
        }
    }
};

// ============================================================================
// ReflectiveEntity — 反射实体基类
// ============================================================================
///
/// @brief 支持运行时属性反射的实体基类
///
/// 存储属性映射表，提供统一的 getProperty(name) / setProperty(name, value) 接口。
/// 子类在构造函数中通过 SC_PROPERTY 宏注册属性。
///
/// @par 使用示例
/// @code
/// class User : public ReflectiveEntity {
/// public:
///     User() {
///         SC_PROPERTY(User, id, getId, setId);
///         SC_PROPERTY(User, name, getName, setName);
///     }
///     int getId() const { return m_id; }
///     void setId(int v) { m_id = v; }
///     QString getName() const { return m_name; }
///     void setName(const QString& v) { m_name = v; }
/// private:
///     int m_id = 0;
///     QString m_name;
/// };
/// @endcode
///
/// @thread_safety 非线程安全 — 属性注册应在单线程中完成
class ReflectiveEntity {
public:
    virtual ~ReflectiveEntity() = default;

    /// @brief 获取属性值
    /// @param name 属性名
    /// @return 属性值；若属性不存在返回 std::nullopt
    ///
    /// [v2.5.1] 返回 std::optional<QVariant> 而非 QVariant()，
    /// 明确区分"属性不存在"和"属性值为空 QVariant"
    std::optional<QVariant> getProperty(const QString& name) const {
        auto it = m_properties.find(name);
        if (it != m_properties.end()) {
            return it->second->getValue(this);
        }
        return std::nullopt;
    }

    /// @brief 设置属性值
    /// @param name 属性名
    /// @param value 属性值
    /// @return true=设置成功，false=属性不存在
    bool setProperty(const QString& name, const QVariant& value) {
        auto it = m_properties.find(name);
        if (it != m_properties.end()) {
            it->second->setValue(this, value);
            return true;
        }
        return false;
    }

    /// @brief 检查属性是否存在
    /// @param name 属性名
    /// @return true=属性已注册
    [[nodiscard]] bool hasProperty(const QString& name) const {
        return m_properties.find(name) != m_properties.end();
    }

    /// @brief 获取所有已注册的属性名
    /// @return 属性名列表
    [[nodiscard]] QStringList propertyNames() const {
        QStringList names;
        for (const auto& [name, _] : m_properties) {
            names.append(name);
        }
        return names;
    }

protected:
    /// @brief 注册属性（由子类构造函数调用）
    /// @tparam EntityType 实体类型
    /// @tparam ValueType 属性值类型
    /// @param name 属性名
    /// @param getter 取值函数
    /// @param setter 设值函数
    template<typename EntityType, typename ValueType>
    void registerProperty(const QString& name,
                          std::function<ValueType(const EntityType&)> getter,
                          std::function<void(EntityType&, const ValueType&)> setter) {
        m_properties[name] = std::make_unique<EntityProperty<EntityType, ValueType>>(
            name, std::move(getter), std::move(setter));
    }

private:
    std::map<QString, std::unique_ptr<EntityPropertyBase>> m_properties;
};

// ============================================================================
// SC_PROPERTY 宏 — 简化属性注册
// ============================================================================
///
/// @brief 在 ReflectiveEntity 子类构造函数中注册属性
///
/// 使用 std::bind 绑定成员函数指针，自动推导属性类型。
///
/// @param ClassName 实体类名
/// @param propName  属性名（字符串，不含引号）
/// @param getter    取值的 const 成员函数
/// @param setter    设值的成员函数
///
/// @par 使用示例
/// @code
/// User() {
///     SC_PROPERTY(User, id, getId, setId);
/// }
/// @endcode
///
/// @par 展开结果
/// @code
/// registerProperty<User, decltype(std::declval<User>().getId())>(
///     "id",
///     std::bind(&User::getId, std::placeholders::_1),
///     std::bind(&User::setId, std::placeholders::_1, std::placeholders::_2));
/// @endcode
#define SC_PROPERTY(ClassName, propName, getter, setter) \
    registerProperty<ClassName, decltype(std::declval<ClassName>().getter())>( \
        QStringLiteral(#propName), \
        std::function(std::bind(&ClassName::getter, std::placeholders::_1)), \
        std::function(std::bind(&ClassName::setter, std::placeholders::_1, std::placeholders::_2)))

// ============================================================================
// SC_TABLE / SC_PRIMARY_KEY 宏 — 实体元数据声明 [v2.0.0 新增]
// ============================================================================
//
// 对标 MyBatis-Plus 的 @TableName / @TableId 注解。
// 为 BaseRepository 提供表名和主键信息。
// EntityTraits 通过 SFINAE 检测这些宏，自动映射到对应数据库表。
//
// 用法:
//   class User : public ReflectiveEntity {
//   public:
//       SC_TABLE("users")           // 声明表名
//       SC_PRIMARY_KEY("id")        // 声明主键列名
//       ...
//   };
//
// 若不声明:
//   - 表名默认使用 typeid(Entity).name()
//   - 主键默认使用 "id"

/// @brief 声明实体对应的数据库表名
/// @param TableName 表名（字符串字面量）
#define SC_TABLE(TableName) \
    static const char* _sc_table_name() { return TableName; }

/// @brief 声明实体的主键列名
/// @param ColumnName 主键列名（字符串字面量）
#define SC_PRIMARY_KEY(ColumnName) \
    static const char* _sc_primary_key() { return ColumnName; }

} // namespace data
} // namespace sc

#endif // SOUL_DATA_ORM_REFLECTION_H