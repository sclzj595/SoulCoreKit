#include "soul/network/interceptor/auth_interceptor.h"

namespace sc::network {

AuthInterceptor::AuthInterceptor() {}

AuthInterceptor::AuthInterceptor(const QString& token)
    : m_token(token) {}

void AuthInterceptor::onRequest(NetworkMessage& request) {
    if (!m_token.isEmpty()) {
        request.metadata["Authorization"] = QString("Bearer %1").arg(m_token);
    }
}

void AuthInterceptor::onResponse(NetworkMessage& response) {
    Q_UNUSED(response);
}

void AuthInterceptor::setToken(const QString& token) {
    m_token = token;
}

QString AuthInterceptor::token() const {
    return m_token;
}

}