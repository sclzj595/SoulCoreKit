#include "soul/network/http_response.h"

namespace sc {
namespace network {

HttpResponse::HttpResponse() {}

int HttpResponse::statusCode() const { return m_statusCode; }
QByteArray HttpResponse::body() const { return m_body; }
QMap<QString, QString> HttpResponse::headers() const { return m_headers; }

sc::json::Json HttpResponse::json() const {
    auto result = sc::json::deserialize(m_body);
    if (result.isOk()) {
        return result.unwrap();
    }
    return sc::json::Json::object();
}

QString HttpResponse::text() const {
    return QString::fromUtf8(m_body);
}

bool HttpResponse::isSuccess() const {
    return m_statusCode >= 200 && m_statusCode < 300;
}

bool HttpResponse::isError() const {
    return !isSuccess();
}

void HttpResponse::setStatusCode(int code) { m_statusCode = code; }
void HttpResponse::setBody(const QByteArray& body) { m_body = body; }
void HttpResponse::setHeaders(const QMap<QString, QString>& headers) { m_headers = headers; }

void HttpResponse::setError(QNetworkReply::NetworkError error, const QString& errorString) {
    m_networkError = error;
    m_errorString = errorString;
}

QNetworkReply::NetworkError HttpResponse::networkError() const { return m_networkError; }
QString HttpResponse::errorString() const { return m_errorString; }

} // namespace network
} // namespace sc