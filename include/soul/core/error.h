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
    Error() = default;

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

private:
    ErrorCode m_code = ErrorCode::Unknown;
    QString m_message;
    std::shared_ptr<Error> m_cause;
};

}

#endif
