#ifndef SOUL_CORE_ERROR_H
#define SOUL_CORE_ERROR_H

#include <string>
#include <memory>

namespace sc {

/**
 * @enum ErrorCode
 * @brief 统一错误码枚举
 *
 * 错误码采用分层设计：
 * - 0-99：通用状态码
 * - 100-199：网络相关错误
 * - 200-299：解析/参数相关错误
 * - 300-399：资源/权限相关错误
 * - 500-599：内部错误
 */
enum class ErrorCode {
    Unknown = 0,        ///< 未知错误
    Success = 1,        ///< 成功
    NetworkError = 100, ///< 网络错误
    Timeout = 101,      ///< 请求超时
    ConnectionRefused = 102, ///< 连接被拒绝
    ParseError = 200,   ///< 解析错误
    InvalidArgument = 201, ///< 参数无效
    SerializationError = 202, ///< 序列化错误
    DeserializationError = 203, ///< 反序列化错误
    NotFound = 300,     ///< 资源未找到
    PermissionDenied = 301, ///< 权限拒绝
    AuthError = 302,    ///< 认证错误
    TokenExpired = 303, ///< Token过期
    InternalError = 500, ///< 内部错误
    NotImplemented = 501, ///< 未实现
};

/**
 * @class Error
 * @brief 统一错误类，支持错误链
 *
 * Error 类封装了错误码、错误消息和可选的原因错误（用于错误链）。
 * 支持嵌套错误，便于追踪完整的错误发生路径。
 *
 * @see ErrorCode
 */
class Error {
public:
    /**
     * @brief 默认构造函数
     */
    Error() = default;

    /**
     * @brief 构造函数
     * @param code 错误码
     * @param message 错误描述信息
     */
    Error(ErrorCode code, const std::string& message)
        : m_code(code), m_message(message) {}

    /**
     * @brief 构造函数（带原因错误）
     * @param code 错误码
     * @param message 错误描述信息
     * @param cause 导致此错误的原因错误
     */
    Error(ErrorCode code, const std::string& message, std::shared_ptr<Error> cause)
        : m_code(code), m_message(message), m_cause(std::move(cause)) {}

    /**
     * @brief 获取错误码
     * @return 错误码
     */
    ErrorCode code() const { return m_code; }

    /**
     * @brief 获取错误消息
     * @return 错误描述信息
     */
    const std::string& message() const { return m_message; }

    /**
     * @brief 获取原因错误
     * @return 导致此错误的原因错误，如果没有则返回 nullptr
     */
    std::shared_ptr<Error> cause() const { return m_cause; }

    /**
     * @brief 转换为字符串表示
     * @return 包含错误码和消息的字符串，如果有原因错误则递归包含
     */
    std::string toString() const {
        std::string result = "[" + std::to_string(static_cast<int>(m_code)) + "] " + m_message;
        if (m_cause) {
            result += " -> " + m_cause->toString();
        }
        return result;
    }

private:
    ErrorCode m_code = ErrorCode::Unknown;
    std::string m_message;
    std::shared_ptr<Error> m_cause;
};

}

#endif
