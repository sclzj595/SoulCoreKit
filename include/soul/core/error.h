#ifndef SOUL_CORE_ERROR_H
#define SOUL_CORE_ERROR_H

#include <string>
#include <memory>
#include <QString>

namespace sc {

enum class ErrorCode {
    Ok = 0,
    Unknown = 1,

    NotFound = 100,
    AlreadyExists = 101,
    InvalidArgument = 102,
    InvalidState = 103,
    PermissionDenied = 104,
    Unauthorized = 105,
    AuthError = 106,
    TokenExpired = 107,

    NetworkError = 200,
    Timeout = 201,
    ConnectionRefused = 202,
    SSLHandshakeFailed = 203,
    NotConnected = 204,

    ParseError = 300,
    SerializationError = 301,
    DeserializationError = 302,

    DatabaseError = 400,
    QueryFailed = 401,
    ConstraintViolation = 402,
    ResourceExhausted = 403,

    FileError = 500,
    FileNotFound = 501,
    FileReadError = 502,
    FileWriteError = 503,

    InternalError = 600,
    NotImplemented = 601,
    OutOfMemory = 602,
};

class Error {
public:
    Error() : m_code(ErrorCode::Ok), m_message("") {}

    Error(ErrorCode code, const QString& message)
        : m_code(code), m_message(message) {}

    Error(ErrorCode code, const char* message)
        : m_code(code), m_message(QString::fromUtf8(message)) {}

    Error(ErrorCode code, const std::string& message)
        : m_code(code), m_message(QString::fromStdString(message)) {}

    Error(ErrorCode code, const QString& message, std::shared_ptr<Error> cause)
        : m_code(code), m_message(message), m_cause(std::move(cause)) {}

    Error(ErrorCode code, const char* message, std::shared_ptr<Error> cause)
        : m_code(code), m_message(QString::fromUtf8(message)), m_cause(std::move(cause)) {}

    Error(ErrorCode code, const std::string& message, std::shared_ptr<Error> cause)
        : m_code(code), m_message(QString::fromStdString(message)), m_cause(std::move(cause)) {}

    ErrorCode code() const { return m_code; }

    const QString& message() const { return m_message; }

    std::shared_ptr<Error> cause() const { return m_cause; }

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

    /// @brief 获取错误码对应的人类可读描述 [v1.9.2 新增]
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
};

}

#endif
