// ============================================================================
// cs_form_validator.cpp — CS 表单校验器实现 [v2.1.0]
// ============================================================================

#include "soul/cs/cs_form_validator.h"

namespace sc::cs {

CsError CsFormValidator::validateField(const QString& fieldName, const QVariant& value) {
    Q_UNUSED(fieldName)
    Q_UNUSED(value)
    // 默认实现：无校验，通过
    return CsError();
}

// ============================================================================
// CompositeFormValidator
// ============================================================================

void CompositeFormValidator::addValidator(std::shared_ptr<CsFormValidator> validator) {
    if (validator) {
        m_validators.push_back(std::move(validator));
    }
}

CsError CompositeFormValidator::validate(const QVariantMap& formData) {
    for (const auto& validator : m_validators) {
        if (!validator) continue;
        CsError error = validator->validate(formData);
        if (!error.isOk()) {
            return error;
        }
    }
    return CsError();
}

} // namespace sc::cs