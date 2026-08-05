#ifndef SOUL_CS_DATA_BINDING_H
#define SOUL_CS_DATA_BINDING_H

// ============================================================================
// cs_data_binding.h — CS 数据绑定引擎 [v2.5.0]
// ============================================================================
//
// 对标 Spring 的 DataBinder + BeanWrapper。
// 基于 ReflectiveEntity 的反射机制，实现 Entity ↔ ViewModel ↔ View 双向绑定。
//
// 关系: 依赖 sc::data::ReflectiveEntity 进行属性反射。
// ============================================================================

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QPointer>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "soul/cs/cs_global.h"
#include "soul/cs/cs_error.h"
#include "soul/cs/cs_view_model.h"
#include "soul/data/orm_reflection.h"

namespace sc::cs {

class CsViewModel;

/// @brief 类型转换器接口
///
/// 用于在 Entity 属性值与 ViewModel 属性值之间进行类型转换。
/// 对标 Spring 的 PropertyEditor / Converter。
class ICsTypeConverter {
public:
    virtual ~ICsTypeConverter() = default;

    /// @brief 将 Entity 值转换为 ViewModel 值
    virtual QVariant toViewModel(const QVariant& entityValue) = 0;

    /// @brief 将 ViewModel 值转换为 Entity 值
    virtual QVariant toEntity(const QVariant& viewModelValue) = 0;
};

/// @brief 内置类型转换器：QString ↔ int
///
/// 对标 Spring 的 StringToNumberConverterFactory。
class StringToIntConverter : public ICsTypeConverter {
public:
    QVariant toViewModel(const QVariant& entityValue) override {
        return entityValue.toString();
    }
    QVariant toEntity(const QVariant& viewModelValue) override {
        bool ok = false;
        int val = viewModelValue.toInt(&ok);
        return ok ? QVariant(val) : QVariant(0);
    }
};

/// @brief 内置类型转换器：QString ↔ double
class StringToDoubleConverter : public ICsTypeConverter {
public:
    QVariant toViewModel(const QVariant& entityValue) override {
        return entityValue.toString();
    }
    QVariant toEntity(const QVariant& viewModelValue) override {
        bool ok = false;
        double val = viewModelValue.toDouble(&ok);
        return ok ? QVariant(val) : QVariant(0.0);
    }
};

/// @brief 内置类型转换器：QString ↔ bool
class StringToBoolConverter : public ICsTypeConverter {
public:
    QVariant toViewModel(const QVariant& entityValue) override {
        return entityValue.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    QVariant toEntity(const QVariant& viewModelValue) override {
        QString s = viewModelValue.toString().toLower();
        return QVariant(s == "true" || s == "1" || s == "yes");
    }
};

/// @brief 内置类型转换器：QDateTime ↔ QString (ISO 8601)
class DateTimeToStringConverter : public ICsTypeConverter {
public:
    QVariant toViewModel(const QVariant& entityValue) override {
        return entityValue.toDateTime().toString(Qt::ISODate);
    }
    QVariant toEntity(const QVariant& viewModelValue) override {
        return QVariant(QDateTime::fromString(viewModelValue.toString(), Qt::ISODate));
    }
};

/// @brief CS 数据绑定引擎（对标 Spring 的 DataBinder）
///
/// 基于属性名称的映射，实现 Entity ↔ ViewModel 双向绑定。
/// 支持自定义类型转换器注册。
///
/// @par 使用示例
/// @code
/// CsDataBinding binding;
/// binding.bindEntity(userEntity, userViewModel);
/// userViewModel.setValue("name", "New Name");
/// binding.syncToEntity();  // userEntity.name = "New Name"
/// @endcode
class CsDataBinding : public QObject {
    Q_OBJECT

public:
    explicit CsDataBinding(QObject* parent = nullptr);
    ~CsDataBinding() override = default;

    /// @brief 绑定 Entity → ViewModel（执行同步，不存储 entity 指针）
    /// @tparam Entity 实体类型（需继承 ReflectiveEntity）
    /// @param entity 实体对象
    /// @param viewModel ViewModel 对象
    ///
    /// 将 Entity 的所有属性值同步到 ViewModel，并存储 ViewModel 引用用于后续 syncToEntity。
    template<typename Entity>
    void bindEntity(const Entity& entity, CsViewModel* viewModel) {
        if (!viewModel) return;

        const auto* reflectiveEntity = dynamic_cast<const sc::data::ReflectiveEntity*>(&entity);
        if (!reflectiveEntity) {
            emit bindingError(CsError(CsErrorCode::ValidationFailed,
                                      "Entity does not implement ReflectiveEntity"));
            return;
        }

        const auto& propNames = reflectiveEntity->propertyNames();
        for (const auto& propName : propNames) {
            QString vmProp = propName;
            auto mappingIt = m_mappings.find(propName.toStdString());
            if (mappingIt != m_mappings.end()) {
                vmProp = mappingIt->second;
            }

            QVariant value = reflectiveEntity->getProperty(propName);
            value = convertValue(propName, value, true);  // toViewModel
            viewModel->set(vmProp, value);
        }

        // 仅存储 ViewModel 引用（QPointer 安全），不存储 entity 指针
        m_boundViewModel = viewModel;

        emit bindingCompleted();
    }

    /// @brief 绑定 ViewModel → Entity（执行同步，不存储 entity 指针）
    /// @tparam Entity 实体类型
    /// @param viewModel ViewModel 对象
    /// @param entity 实体对象（输出）
    ///
    /// 将 ViewModel 的属性值同步回 Entity，并存储 ViewModel 引用用于后续 syncToEntity。
    template<typename Entity>
    void bindViewModel(const CsViewModel* viewModel, Entity& entity) {
        if (!viewModel) return;

        auto* reflectiveEntity = dynamic_cast<sc::data::ReflectiveEntity*>(&entity);
        if (!reflectiveEntity) {
            emit bindingError(CsError(CsErrorCode::ValidationFailed,
                                      "Entity does not implement ReflectiveEntity"));
            return;
        }

        const auto& propNames = reflectiveEntity->propertyNames();
        for (const auto& propName : propNames) {
            QString vmProp = propName;
            auto mappingIt = m_mappings.find(propName.toStdString());
            if (mappingIt != m_mappings.end()) {
                vmProp = mappingIt->second;
            }

            QVariant value = viewModel->get(vmProp);
            value = convertValue(propName, value, false);  // toEntity
            reflectiveEntity->setProperty(propName, value);
        }

        // 仅存储 ViewModel 引用（QPointer 安全），不存储 entity 指针
        m_boundViewModel = const_cast<CsViewModel*>(viewModel);

        emit bindingCompleted();
    }

    /// @brief 同步 Entity → ViewModel（显式传入 entity，避免悬垂指针）
    /// @tparam Entity 实体类型（需继承 ReflectiveEntity）
    /// @param entity 实体对象（const 引用，仅读取）
    ///
    /// 对标 Spring 的 DataBinder.write()。
    /// entity 参数在调用点传入，不存储指针，消除生命周期风险。
    template<typename Entity>
    void syncToViewModel(const Entity& entity);

    /// @brief 同步 ViewModel → Entity（显式传入 entity，避免悬垂指针）
    /// @tparam Entity 实体类型（需继承 ReflectiveEntity）
    /// @param entity 实体对象（非 const 引用，会被修改）
    ///
    /// 对标 Spring 的 DataBinder.bind()。
    /// entity 参数在调用点传入，不存储指针，消除生命周期风险。
    template<typename Entity>
    void syncToEntity(Entity& entity);

    /// @brief 注册类型转换器
    /// @param propertyName 属性名
    /// @param converter 转换器
    void registerConverter(const QString& propertyName, std::shared_ptr<ICsTypeConverter> converter);

    /// @brief 添加属性映射
    /// @param entityProperty Entity 属性名
    /// @param viewModelProperty ViewModel 属性名
    void addMapping(const QString& entityProperty, const QString& viewModelProperty);

    /// @brief 清除所有映射
    void clearMappings();

signals:
    /// @brief 绑定完成信号
    void bindingCompleted();

    /// @brief 绑定错误信号
    void bindingError(const CsError& error);

private:
    /// @brief 执行类型转换
    QVariant convertValue(const QString& propertyName, const QVariant& value, bool toViewModel);

    std::map<std::string, QString> m_mappings;  // entityProperty → viewModelProperty
    std::map<std::string, std::shared_ptr<ICsTypeConverter>> m_converters;

    /// @brief 已绑定的 ViewModel 指针（QPointer 安全追踪）
    ///
    /// 对标 Spring 的 BeanWrapperImpl 内部持有的 target object 引用。
    /// 使用 QPointer 确保 ViewModel 销毁后自动置空，避免悬垂指针。
    QPointer<CsViewModel> m_boundViewModel;
};

// ============================================================================
// 模板实现 — syncToViewModel / syncToEntity
// ============================================================================

template<typename Entity>
void CsDataBinding::syncToViewModel(const Entity& entity) {
    if (!m_boundViewModel) {
        emit bindingError(CsError(CsErrorCode::ValidationFailed,
                                  "syncToViewModel: no binding state. Call bindEntity() first."));
        return;
    }

    const auto* reflectiveEntity = dynamic_cast<const sc::data::ReflectiveEntity*>(&entity);
    if (!reflectiveEntity) {
        emit bindingError(CsError(CsErrorCode::ValidationFailed,
                                  "Entity does not implement ReflectiveEntity"));
        return;
    }

    const auto& propNames = reflectiveEntity->propertyNames();
    for (const auto& propName : propNames) {
        QString vmProp = propName;
        auto mappingIt = m_mappings.find(propName.toStdString());
        if (mappingIt != m_mappings.end()) {
            vmProp = mappingIt->second;
        }

        QVariant value = reflectiveEntity->getProperty(propName);
        value = convertValue(propName, value, true);  // toViewModel
        m_boundViewModel->set(vmProp, value);
    }

    emit bindingCompleted();
}

template<typename Entity>
void CsDataBinding::syncToEntity(Entity& entity) {
    if (!m_boundViewModel) {
        emit bindingError(CsError(CsErrorCode::ValidationFailed,
                                  "syncToEntity: no binding state. Call bindViewModel() first."));
        return;
    }

    auto* reflectiveEntity = dynamic_cast<sc::data::ReflectiveEntity*>(&entity);
    if (!reflectiveEntity) {
        emit bindingError(CsError(CsErrorCode::ValidationFailed,
                                  "Entity does not implement ReflectiveEntity"));
        return;
    }

    const auto& propNames = reflectiveEntity->propertyNames();
    for (const auto& propName : propNames) {
        QString vmProp = propName;
        auto mappingIt = m_mappings.find(propName.toStdString());
        if (mappingIt != m_mappings.end()) {
            vmProp = mappingIt->second;
        }

        QVariant value = m_boundViewModel->get(vmProp);
        value = convertValue(propName, value, false);  // toEntity
        reflectiveEntity->setProperty(propName, value);
    }

    emit bindingCompleted();
}

} // namespace sc::cs

#endif // SOUL_CS_DATA_BINDING_H