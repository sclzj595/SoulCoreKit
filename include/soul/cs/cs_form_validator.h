#ifndef SOUL_CS_FORM_VALIDATOR_H
#define SOUL_CS_FORM_VALIDATOR_H

// ============================================================================
// cs_form_validator.h — CS 表单校验器 [v2.5.0]
// ============================================================================
//
// 对标 Spring 的 Validator 接口 + @Valid 注解。
// 提供声明式表单校验，与 sc::validation::Validator 互操作。
//
// 关系: 扩展 sc::validation::Validator，增加 CS 场景的表单校验支持。
// ============================================================================

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "soul/cs/cs_global.h"
#include "soul/cs/cs_error.h"

namespace sc::cs {

/// @brief CS 表单校验器接口（对标 Spring 的 Validator 接口）
///
/// 提供表单数据的校验能力。
/// 对标 Spring 的 org.springframework.validation.Validator。
///
/// @par 使用示例
/// @code
/// class UserFormValidator : public CsFormValidator {
/// public:
///     Result<void> validate(const QVariantMap& formData) override {
///         if (formData["name"].toString().isEmpty()) {
///             return CsError(CsErrorCode::ValidationFailed, "用户名不能为空");
///         }
///         if (formData["age"].toInt() < 0) {
///             return CsError(CsErrorCode::ValidationFailed, "年龄不能为负数");
///         }
///         return {};
///     }
/// };
/// @endcode
class CsFormValidator {
public:
    virtual ~CsFormValidator() = default;

    /// @brief 校验表单数据
    /// @param formData 表单数据
    /// @return 校验通过返回空 Error，失败返回具体错误
    virtual CsError validate(const QVariantMap& formData) = 0;

    /// @brief 校验单个字段
    /// @param fieldName 字段名
    /// @param value 字段值
    /// @return 校验通过返回空 Error，失败返回具体错误
    virtual CsError validateField(const QString& fieldName, const QVariant& value);

    /// @brief 获取校验器名称
    virtual QString validatorName() const { return "CsFormValidator"; }
};

/// @brief 组合校验器
///
/// 将多个校验器组合在一起，按顺序执行校验。
/// 对标 Spring 的 SpringValidatorAdapter。
class CompositeFormValidator : public CsFormValidator {
public:
    /// @brief 添加校验器
    void addValidator(std::shared_ptr<CsFormValidator> validator);

    /// @brief 校验所有注册的校验器
    CsError validate(const QVariantMap& formData) override;

    /// @brief 获取校验器名称
    QString validatorName() const override { return "CompositeFormValidator"; }

private:
    std::vector<std::shared_ptr<CsFormValidator>> m_validators;
};

} // namespace sc::cs

#endif // SOUL_CS_FORM_VALIDATOR_H