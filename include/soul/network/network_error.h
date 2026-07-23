/**
 * @file network_error.h
 * @brief 网络错误类
 * @details 继承 Error 基类，定义网络模块专用错误码和错误处理
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_NETWORK_ERROR_H
#define SOUL_NETWORK_NETWORK_ERROR_H

#include "soul/network/network_global.h"
#include <QString>
#include "soul/core/error.h"

namespace sc {
namespace network {

enum class NetworkErrorCode {
    Unknown = 1000,
    ConnectionRefused = 1001,
    RemoteHostClosed = 1002,
    HostNotFound = 1003,
    Timeout = 1004,
    SslHandshakeFailed = 1005,
    ProxyAuthenticationRequired = 1006,
    ContentNotFound = 1007,
    InvalidResponse = 1008,
    RequestCancelled = 1009,
    TooManyRedirects = 1010,
    NotImplemented = 1011
};

class SC_NETWORK_EXPORT NetworkError : public Error {
public:
    NetworkError(NetworkErrorCode code, const QString& message);
    NetworkError(NetworkErrorCode code, const QString& message, std::shared_ptr<Error> cause);

    NetworkErrorCode networkCode() const;
    int httpStatusCode() const;
    void setHttpStatusCode(int code);

private:
    NetworkErrorCode m_networkCode;
    int m_httpStatusCode = 0;
};

} // namespace network
} // namespace sc

#endif