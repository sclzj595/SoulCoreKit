#ifndef SOUL_CORE_ERROR_H
#define SOUL_CORE_ERROR_H

#include <string>
#include <memory>
#include <QString>
#include <QHash>
#include <QVariant>

namespace sc {

// ============================================================================
// ErrorCode — 统一错误码枚举 [v2.6.0]
// ============================================================================
// 按类别分段: 1xx 资源, 2xx 网络, 3xx 解析, 4xx 数据库, 5xx 文件, 6xx 内部

enum class ErrorCode {
    Ok = 0,
    Unknown = 1,

    // 1xx: 资源 / 参数 / 状态 / 权限
    NotFound = 100,
    AlreadyExists = 101,
    InvalidArgument = 102,
    InvalidState = 103,
    PermissionDenied = 104,
    Unauthorized = 105,
    AuthError = 106,
    TokenExpired = 107,

    // 2xx: 网络 / 通信
    NetworkError = 200,
    Timeout = 201,
    ConnectionRefused = 202,
    SSLHandshakeFailed = 203,
    NotConnected = 204,

    // 3xx: 解析 / 序列化
    ParseError = 300,
    SerializationError = 301,
    DeserializationError = 302,

    // 4xx: 数据库
    DatabaseError = 400,
    QueryFailed = 401,
    ConstraintViolation = 402,
    ResourceExhausted = 403,

    // 5xx: 文件 / IO
    FileError = 500,
    FileNotFound = 501,
    FileReadError = 502,
    FileWriteError = 503,

    // 6xx: 内部 / 系统
    InternalError = 600,
    NotImplemented = 601,
    OutOfMemory = 602,
};

// ============================================================================
// ErrorCategory — 错误类别 [v2.6.0 新增]
// ============================================================================

enum class ErrorCategory {
    None,
    Resource,       // 1xx
    Network,        // 2xx
    Parse,          // 3xx
    Database,       // 4xx
    FileIO,         // 5xx
    Internal,       // 6xx
};

/// @brief 根据 ErrorCode 推断 ErrorCategory
inline ErrorCategory categoryFromCode(ErrorCode code) {
    int c = static_cast<int>(code);
    if (c >= 100 && c < 200) return ErrorCategory::Resource;
    if (c >= 200 && c < 300) return ErrorCategory::Network;
    if (c >= 300 && c < 400) return ErrorCategory::Parse;
    if (c >= 400 && c < 500) return ErrorCategory::Database;
    if (c >= 500 && c < 600) return ErrorCategory::FileIO;
    if (c >= 600)            return ErrorCategory::Internal;
    return ErrorCategory::None;
}

// ============================================================================
// Error — 统一错误类型 [v2.6.0 增强]
// ============================================================================
//
// v2.6.0 新增:
//   - ErrorCategory 类别自动推断
//   - Context 元数据 (key-value)，支持附加请求 ID、用户 ID 等诊断信息
//   - isCategory() 便捷判断
//   - withContext() 链式添加上下文
//
// 设计原则:
//   - 不可变值类型: 构造后不可修改
//   - 链式 cause: 通过 shared_ptr<Error> 支持错误链
//   - Context: 可选诊断元数据，不参与相等比较
//   - 轻量: 无 context 时不分配额外内存

class Error {
public:
    // === 构造 ===

    Error() : m_code(ErrorCode::Ok), m_message(""), m_category(ErrorCategory::None) {}

    Error(ErrorCode code, const QString& message)
        : m_code(code), m_message(message), m_category(categoryFromCode(code)) {}

    Error(ErrorCode code, const char* message)
        : m_code(code), m_message(QString::fromUtf8(message)), m_category(categoryFromCode(code)) {}

    Error(ErrorCode code, const std::string& message)
        : m_code(code), m_message(QString::fromStdString(message)), m_category(categoryFromCode(code)) {}

    Error(ErrorCode code, const QString& message, std::shared_ptr<Error> cause)
        : m_code(code), m_message(message), m_cause(std::move(cause)), m_category(categoryFromCode(code)) {}

    Error(ErrorCode code, const char* message, std::shared_ptr<Error> cause)
        : m_code(code), m_message(QString::fromUtf8(message)), m_cause(std::move(cause)), m_category(categoryFromCode(code)) {}

    Error(ErrorCode code, const std::string& message, std::shared_ptr<Error> cause)
        : m_code(code), m_message(QString::fromStdString(message)), m_cause(std::move(cause)), m_category(categoryFromCode(code)) {}

    // === 访问器 ===

    ErrorCode code() const { return m_code; }
    const QString& message() const { return m_message; }
    std::shared_ptr<Error> cause() const { return m_cause; }

    /// @brief 错误类别 [v2.6.0 新增]
    ErrorCategory category() const { return m_category; }

    /// @brief 便捷判断类别 [v2.6.0 新增]
    bool isCategory(ErrorCategory cat) const { return m_category == cat; }
    bool isNetworkError() const { return m_category == ErrorCategory::Network; }
    bool isDatabaseError() const { return m_category == ErrorCategory::Database; }
    bool isResourceError() const { return m_category == ErrorCategory::Resource; }
    bool isFileError() const { return m_category == ErrorCategory::FileIO; }
    bool isParseError() const { return m_category == ErrorCategory::Parse; }
    bool isInternalError() const { return m_category == ErrorCategory::Internal; }

    // === Context 元数据 [v2.6.0 新增] ===

    /// @brief 获取诊断上下文 (request_id, user_id, etc.)
    const QHash<QString, QVariant>& context() const { return m_context; }

    /// @brief 链式添加上下文
    Error withContext(const QString& key, const QVariant& value) const {
        Error copy = *this;
        copy.m_context.insert(key, value);
        return copy;
    }

    /// @brief 获取上下文值
    QVariant contextValue(const QString& key) const {
        return m_context.value(key);
    }

    // === 格式化 ===

    QString toString() const {
        QString result = QString("[%1] %2").arg(static_cast<int>(m_code)).arg(m_message);
        if (m_cause) {
            result += " -> " + m_cause->toString();
        }
        return result;
    }

    std::string toStdString() const {
        return toString().toStdString();
    }

    /// @brief 获取错误码对应的人类可读描述
    static QString errorDescription(ErrorCode code) {
        switch (code) {
        case ErrorCode::Ok:                 return QStringLiteral("Success");
        case ErrorCode::Unknown:            return QStringLiteral("Unknown error");
        case ErrorCode::NotFound:           return QStringLiteral("Resource not found");
        case ErrorCode::AlreadyExists:      return QStringLiteral("Resource already exists");
        case ErrorCode::InvalidArgument:    return QStringLiteral("Invalid argument");
        case ErrorCode::InvalidState:       return QStringLiteral("Invalid state");
        case ErrorCode::PermissionDenied:   return QStringLiteral("Permission denied");
        case ErrorCode::Unauthorized:       return QStringLiteral("Unauthorized");
        case ErrorCode::AuthError:          return QStringLiteral("Authentication error");
        case ErrorCode::TokenExpired:       return QStringLiteral("Token expired");
        case ErrorCode::NetworkError:       return QStringLiteral("Network error");
        case ErrorCode::Timeout:            return QStringLiteral("Operation timed out");
        case ErrorCode::ConnectionRefused:  return QStringLiteral("Connection refused");
        case ErrorCode::SSLHandshakeFailed: return QStringLiteral("SSL handshake failed");
        case ErrorCode::NotConnected:       return QStringLiteral("Not connected");
        case ErrorCode::ParseError:         return QStringLiteral("Parse error");
        case ErrorCode::SerializationError: return QStringLiteral("Serialization error");
        case ErrorCode::DeserializationError: return QStringLiteral("Deserialization error");
        case ErrorCode::DatabaseError:      return QStringLiteral("Database error");
        case ErrorCode::QueryFailed:        return QStringLiteral("Query failed");
        case ErrorCode::ConstraintViolation: return QStringLiteral("Constraint violation");
        case ErrorCode::ResourceExhausted:  return QStringLiteral("Resource exhausted");
        case ErrorCode::FileError:          return QStringLiteral("File error");
        case ErrorCode::FileNotFound:       return QStringLiteral("File not found");
        case ErrorCode::FileReadError:      return QStringLiteral("File read error");
        case ErrorCode::FileWriteError:     return QStringLiteral("File write error");
        case ErrorCode::InternalError:      return QStringLiteral("Internal error");
        case ErrorCode::NotImplemented:     return QStringLiteral("Not implemented");
        case ErrorCode::OutOfMemory:        return QStringLiteral("Out of memory");
        default:                            return QStringLiteral("Unrecognized error code");
        }
    }

private:
    ErrorCode m_code = ErrorCode::Unknown;
    QString m_message;
    std::shared_ptr<Error> m_cause;
    ErrorCategory m_category = ErrorCategory::None;
    QHash<QString, QVariant> m_context;  // v2.6.0: 诊断元数据
};

} // namespace sc

#endif // SOUL_CORE_ERROR_H
