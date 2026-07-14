#include "soul/network/http_request.h"
#include <QUrlQuery>

namespace sc {

HttpRequest::HttpRequest() {}

HttpRequest::HttpRequest(HttpMethod method, const QUrl& url)
    : m_method(method), m_url(url) {}

HttpMethod HttpRequest::method() const { return m_method; }
QUrl HttpRequest::url() const { return m_url; }

QUrl HttpRequest::buildUrl() const {
    QUrl result = m_url;
    QUrlQuery query;

    for (auto it = m_params.constBegin(); it != m_params.constEnd(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }

    result.setQuery(query);
    return result;
}

HttpRequest& HttpRequest::setMethod(HttpMethod method) {
    m_method = method;
    return *this;
}

HttpRequest& HttpRequest::setUrl(const QUrl& url) {
    m_url = url;
    return *this;
}

HttpRequest& HttpRequest::addHeader(const QString& key, const QString& value) {
    m_headers[key] = value;
    return *this;
}

HttpRequest& HttpRequest::setHeaders(const QMap<QString, QString>& headers) {
    m_headers = headers;
    return *this;
}

QMap<QString, QString> HttpRequest::headers() const { return m_headers; }

HttpRequest& HttpRequest::addParam(const QString& key, const QString& value) {
    m_params[key] = value;
    return *this;
}

HttpRequest& HttpRequest::setParams(const QMap<QString, QString>& params) {
    m_params = params;
    return *this;
}

QMap<QString, QString> HttpRequest::params() const { return m_params; }

HttpRequest& HttpRequest::setBody(const QByteArray& body) {
    m_body = body;
    return *this;
}

HttpRequest& HttpRequest::setJsonBody(const QJsonDocument& json) {
    m_body = json.toJson(QJsonDocument::Compact);
    m_headers["Content-Type"] = "application/json";
    return *this;
}

QByteArray HttpRequest::body() const { return m_body; }

HttpRequest& HttpRequest::setTimeout(int ms) {
    m_timeout = ms;
    return *this;
}

int HttpRequest::timeout() const { return m_timeout; }

}