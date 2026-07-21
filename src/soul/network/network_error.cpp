#include "soul/network/network_error.h"

namespace sc {
namespace network {

NetworkError::NetworkError(NetworkErrorCode code, const QString& message)
    : Error(static_cast<ErrorCode>(code), message.toStdString()),
      m_networkCode(code) {}

NetworkError::NetworkError(NetworkErrorCode code, const QString& message, std::shared_ptr<Error> cause)
    : Error(static_cast<ErrorCode>(code), message.toStdString(), cause),
      m_networkCode(code) {}

NetworkErrorCode NetworkError::networkCode() const {
    return m_networkCode;
}

int NetworkError::httpStatusCode() const {
    return m_httpStatusCode;
}

void NetworkError::setHttpStatusCode(int code) {
    m_httpStatusCode = code;
}

} // namespace network
} // namespace sc