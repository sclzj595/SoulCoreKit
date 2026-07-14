#ifndef SOUL_NETWORK_NETWORK_ERROR_H
#define SOUL_NETWORK_NETWORK_ERROR_H

#include <QString>
#include "soul/core/error.h"

namespace sc {

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

class NetworkError : public Error {
public:
    NetworkError(NetworkErrorCode code, const QString& message);
    NetworkError(NetworkErrorCode code, const QString& message, std::unique_ptr<Error> cause);

    NetworkErrorCode networkCode() const;
    int httpStatusCode() const;
    void setHttpStatusCode(int code);

private:
    NetworkErrorCode m_networkCode;
    int m_httpStatusCode = 0;
};

}

#endif