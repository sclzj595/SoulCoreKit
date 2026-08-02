#ifndef SOUL_VALIDATION_VALIDATOR_H
#define SOUL_VALIDATION_VALIDATOR_H

// ============================================================================
// validator.h — 声明式输入验证框架 [v1.9.3]
// ============================================================================
//
// 设计目标: 对标 javax.validation / FluentValidation,提供声明式字段校验,
// 防止 SQL 注入/XSS/参数越界等安全问题。
//
// 核心概念:
//   - Validator: 校验器,绑定一组 ValidationRule
//   - ValidationRule: 单条校验规则(required/range/length/pattern/email/custom)
//   - ValidationResult: 校验结果,包含所有失败规则的信息
//
// 用法:
//   Validator validator;
//   validator.required("username", username, "用户名不能为空")
//            .range("age", age, 0, 150, "年龄必须在0-150之间")
//            .length("password", password, 6, 128, "密码长度6-128位")
//            .pattern("email", email, R"(^[\w.-]+@[\w.-]+\.\w+$)", "邮箱格式不正确")
//            .email("contact", contact, "邮箱格式不正确");
//
//   auto result = validator.validate();
//   if (!result.isValid()) {
//       for (const auto& err : result.errors()) {
//           SC_WARN(err.toString());
//       }
//       return Result<void>::err(Error(ErrorCode::InvalidArgument, result.firstError()));
//   }
//
// 宏方式(编译期绑定,零运行时开销):
//   struct LoginRequest {
//       QString username;
//       int age = 0;
//   };
//   SC_VALIDATE_BIND(LoginRequest,
//       SC_VALIDATE_FIELD("username", required, "用户名不能为空")
//       SC_VALIDATE_FIELD("age", range(0, 150), "年龄必须在0-150之间")
//   )
//
//   auto result = Validator::validate<LoginRequest>(req);
//
// @thread_safety 非线程安全,每个 Validator 实例应在单线程中使用

#include <cmath>
#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <type_traits>
#include <vector>
#include <QString>
#include <QStringList>

namespace sc {
namespace validation {

// ============================================================================
// ValidationError — 单条校验错误
// ============================================================================
struct ValidationError {
    std::string field;       ///< 字段名
    std::string rule;        ///< 规则名(required/range/length/pattern/email/custom)
    std::string message;     ///< 错误消息
    std::string value;       ///< 实际值(截断到 50 字符)

    std::string toString() const {
        return field + "[" + rule + "]: " + message + " (value=" + value + ")";
    }
};

// ============================================================================
// ValidationResult — 校验结果
// ============================================================================
class ValidationResult {
public:
    bool isValid() const { return m_errors.empty(); }
    bool hasErrors() const { return !m_errors.empty(); }

    const std::vector<ValidationError>& errors() const { return m_errors; }

    /// @brief 获取第一条错误消息
    std::string firstError() const {
        return m_errors.empty() ? "" : m_errors[0].message;
    }

    /// @brief 获取指定字段的所有错误
    std::vector<ValidationError> fieldErrors(const std::string& field) const {
        std::vector<ValidationError> result;
        for (const auto& e : m_errors) {
            if (e.field == field) result.push_back(e);
        }
        return result;
    }

    void addError(ValidationError error) {
        m_errors.push_back(std::move(error));
    }

private:
    std::vector<ValidationError> m_errors;
};

// ============================================================================
// Validator — 校验器
// ============================================================================

/// @brief safeString() 选项 [v1.9.3]
struct SafeStringOptions {
    bool allowQuotes     = false;  ///< 允许单引号和双引号
    bool allowSemicolon  = false;  ///< 允许分号
    bool allowDashDash   = false;  ///< 允许 "--" (SQL 注释)
    bool allowSlashStar  = false;  ///< 允许 "/*" (SQL 块注释)
    bool checkXss        = true;   ///< 是否检查 XSS 特征
};

class Validator {
public:
    Validator() = default;

    /// @brief 执行所有已注册规则的校验
    ValidationResult validate() const;

    // ============================================================
    // 内置校验规则
    // ============================================================

    /// @brief 必填校验(非空字符串)
    Validator& required(const std::string& field, const QString& value,
                        const std::string& message = "");

    /// @brief 必填校验(非空 std::string)
    Validator& required(const std::string& field, const std::string& value,
                        const std::string& message = "");

    /// @brief 数值范围校验
    template<typename T>
    Validator& range(const std::string& field, T value, T min, T max,
                     const std::string& message = "") {
        m_rules.push_back([=]() -> std::optional<ValidationError> {
            if constexpr (std::is_floating_point_v<T>) {
                if (std::isnan(value)) {
                    return ValidationError{
                        field, "range",
                        message.empty()
                            ? field + " must be between " + std::to_string(min) + " and " + std::to_string(max)
                            : message,
                        std::to_string(value)
                    };
                }
            }
            if (value < min || value > max) {
                return ValidationError{
                    field, "range",
                    message.empty()
                        ? field + " must be between " + std::to_string(min) + " and " + std::to_string(max)
                        : message,
                    std::to_string(value)
                };
            }
            return std::nullopt;
        });
        return *this;
    }

    /// @brief 字符串长度校验
    Validator& length(const std::string& field, const QString& value,
                      int minLen, int maxLen, const std::string& message = "");

    /// @brief 正则表达式校验
    Validator& pattern(const std::string& field, const QString& value,
                       const std::string& regex, const std::string& message = "");

    /// @brief 邮箱格式校验
    Validator& email(const std::string& field, const QString& value,
                     const std::string& message = "");

    /// @brief 自定义校验规则
    /// @param ruleName 规则名称
    /// @param check 校验函数,返回 std::nullopt 表示通过,返回 ValidationError 表示失败
    Validator& custom(const std::string& field, const std::string& ruleName,
                      std::function<std::optional<ValidationError>()> check);

    // ============================================================
    // 安全校验(SQL注入 / XSS 防护)
    // ============================================================

    /// @brief 安全字符串校验(防止 SQL 注入 + XSS)
    /// 拒绝包含以下字符的输入: ; ' " < > -- /* */
    ///
    /// @warning 本方法仅为基础辅助检查,采用黑名单方式,无法保证完全防御。
    ///          不能替代参数化查询(parameterized queries)和输出编码(output encoding)。
    ///          生产环境必须使用参数化查询防止 SQL 注入,使用 HTML 转义防止 XSS。
    /// @note 黑名单方式可能误判合法文本,例如含单引号的姓名("O'Brien")、
    ///       含双引号的引用文本、含分号的列表等。对于允许此类字符的场景,
    ///       请通过 options 参数放宽限制,或使用更精确的白名单方式。
    /// @param options 选项,可允许特定字符类(如 allowQuotes/allowSemicolon)
    Validator& safeString(const std::string& field, const QString& value,
                          const std::string& message = "",
                          SafeStringOptions options = {});

    /// @brief 纯数字校验(仅允许 0-9)
    Validator& digitsOnly(const std::string& field, const QString& value,
                          const std::string& message = "");

    /// @brief 字母数字校验(仅允许 a-zA-Z0-9_)
    Validator& alphanumeric(const std::string& field, const QString& value,
                            const std::string& message = "");

    /// @brief 清空所有已注册规则
    void clear() { m_rules.clear(); }

private:
    using RuleFunc = std::function<std::optional<ValidationError>()>;
    std::vector<RuleFunc> m_rules;
};

// ============================================================================
// 预定义的正则表达式
// ============================================================================
namespace patterns {
    constexpr const char* EMAIL = R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)";
    constexpr const char* PHONE = R"(^1[3-9]\d{9}$)";
    constexpr const char* URL   = R"(^https?://[\w.-]+(:\d+)?(/[\w./-]*)?(\?[\w=&.-]*)?(#[\w.-]*)?$)";
    constexpr const char* IP    = R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$)";
    constexpr const char* UUID  = R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)";
    constexpr const char* ALPHANUMERIC = R"(^[a-zA-Z0-9_]+$)";
    constexpr const char* DIGITS = R"(^\d+$)";
}

} // namespace validation
} // namespace sc

#endif // SOUL_VALIDATION_VALIDATOR_H