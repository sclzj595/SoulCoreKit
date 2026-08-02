#include "soul/validation/validator.h"
#include <algorithm>
#include <memory>

namespace sc {
namespace validation {

// ============================================================================
// Validator::validate
// ============================================================================

ValidationResult Validator::validate() const {
    ValidationResult result;
    for (const auto& rule : m_rules) {
        auto error = rule();
        if (error.has_value()) {
            result.addError(std::move(error.value()));
        }
    }
    return result;
}

// ============================================================================
// 内置规则实现
// ============================================================================

static std::string truncateValue(const QString& value) {
    if (value.length() <= 47) return value.toStdString();
    return value.left(47).toStdString() + "...";
}

Validator& Validator::required(const std::string& field, const QString& value,
                                const std::string& message) {
    m_rules.push_back([=]() -> std::optional<ValidationError> {
        if (value.trimmed().isEmpty()) {
            return ValidationError{
                field, "required",
                message.empty() ? field + " is required and must not be empty" : message,
                "(empty)"
            };
        }
        return std::nullopt;
    });
    return *this;
}

Validator& Validator::required(const std::string& field, const std::string& value,
                                const std::string& message) {
    return required(field, QString::fromStdString(value), message);
}

Validator& Validator::length(const std::string& field, const QString& value,
                              int minLen, int maxLen, const std::string& message) {
    m_rules.push_back([=]() -> std::optional<ValidationError> {
        int len = value.length();  // UTF-16 code units (not Unicode code points)
        if (len < minLen || len > maxLen) {
            return ValidationError{
                field, "length",
                message.empty()
                    ? field + " length must be between " + std::to_string(minLen)
                      + " and " + std::to_string(maxLen)
                    : message,
                truncateValue(value)
            };
        }
        return std::nullopt;
    });
    return *this;
}

Validator& Validator::pattern(const std::string& field, const QString& value,
                               const std::string& regex, const std::string& message) {
    auto re = std::make_shared<std::regex>(regex);
    m_rules.push_back([=]() -> std::optional<ValidationError> {
        try {
            if (!std::regex_match(value.toStdString(), *re)) {
                return ValidationError{
                    field, "pattern",
                    message.empty() ? field + " does not match required pattern" : message,
                    truncateValue(value)
                };
            }
        } catch (const std::regex_error& e) {
            return ValidationError{
                field, "pattern",
                "Invalid regex pattern: " + std::string(e.what()),
                regex
            };
        }
        return std::nullopt;
    });
    return *this;
}

Validator& Validator::email(const std::string& field, const QString& value,
                             const std::string& message) {
    return pattern(field, value, patterns::EMAIL,
                   message.empty() ? field + " must be a valid email address" : message);
}

Validator& Validator::custom(const std::string& field, const std::string& ruleName,
                              std::function<std::optional<ValidationError>()> check) {
    m_rules.push_back([=]() -> std::optional<ValidationError> {
        auto result = check();
        if (result.has_value()) {
            result->field = field;
            result->rule = ruleName;
        }
        return result;
    });
    return *this;
}

// ============================================================================
// 安全校验
// ============================================================================

Validator& Validator::safeString(const std::string& field, const QString& value,
                                  const std::string& message, SafeStringOptions options) {
    m_rules.push_back([=]() -> std::optional<ValidationError> {
        std::string s = value.toStdString();

        // 检测 SQL 注入特征(根据 options 跳过部分检查)
        if (!options.allowSemicolon && s.find(';') != std::string::npos) {
            return ValidationError{
                field, "safeString",
                message.empty() ? field + " contains potentially dangerous characters" : message,
                truncateValue(value)
            };
        }
        if (!options.allowQuotes && s.find('\'') != std::string::npos) {
            return ValidationError{
                field, "safeString",
                message.empty() ? field + " contains potentially dangerous characters" : message,
                truncateValue(value)
            };
        }
        if (!options.allowQuotes && s.find('"') != std::string::npos) {
            return ValidationError{
                field, "safeString",
                message.empty() ? field + " contains potentially dangerous characters" : message,
                truncateValue(value)
            };
        }
        if (!options.allowDashDash && s.find("--") != std::string::npos) {
            return ValidationError{
                field, "safeString",
                message.empty() ? field + " contains potentially dangerous characters" : message,
                truncateValue(value)
            };
        }
        if (!options.allowSlashStar && s.find("/*") != std::string::npos) {
            return ValidationError{
                field, "safeString",
                message.empty() ? field + " contains potentially dangerous characters" : message,
                truncateValue(value)
            };
        }

        // 检测 XSS 特征(可关闭)
        if (options.checkXss) {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (lower.find("<script") != std::string::npos ||
                lower.find("javascript:") != std::string::npos ||
                lower.find("onerror=") != std::string::npos ||
                lower.find("onload=") != std::string::npos) {
                return ValidationError{
                    field, "safeString",
                    message.empty() ? field + " contains potentially dangerous XSS content" : message,
                    truncateValue(value)
                };
            }
        }
        return std::nullopt;
    });
    return *this;
}

Validator& Validator::digitsOnly(const std::string& field, const QString& value,
                                  const std::string& message) {
    return pattern(field, value, patterns::DIGITS,
                   message.empty() ? field + " must contain only digits" : message);
}

Validator& Validator::alphanumeric(const std::string& field, const QString& value,
                                    const std::string& message) {
    return pattern(field, value, patterns::ALPHANUMERIC,
                   message.empty() ? field + " must contain only letters, digits and underscores" : message);
}

} // namespace validation
} // namespace sc