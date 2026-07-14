#include "soul/network/http_client.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QEventLoop>
#include <QTimer>

namespace sc {

HttpClient::HttpClient(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
    setupSslConfiguration();
}

HttpClient::~HttpClient() {}

Result<HttpResponse> HttpClient::send(const HttpRequest& request) {
    HttpRequest mutableRequest = request;
    
    for (const auto& interceptor : m_interceptors) {
        interceptor->onRequest(mutableRequest);
    }

    QNetworkRequest qrequest(mutableRequest.buildUrl());

    for (const auto& header : mutableRequest.headers().toStdMap()) {
        qrequest.setRawHeader(header.first.toUtf8(), header.second.toUtf8());
    }

    QNetworkReply* reply = nullptr;
    switch (mutableRequest.method()) {
    case HttpMethod::Get:
        reply = m_manager->get(qrequest);
        break;
    case HttpMethod::Post:
        reply = m_manager->post(qrequest, mutableRequest.body());
        break;
    case HttpMethod::Put:
        reply = m_manager->put(qrequest, mutableRequest.body());
        break;
    case HttpMethod::Delete:
        reply = m_manager->deleteResource(qrequest);
        break;
    default:
        reply = m_manager->get(qrequest);
    }

    QEventLoop loop;
    QTimer::singleShot(mutableRequest.timeout(), &loop, &QEventLoop::quit);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorStr = reply->errorString();
        reply->deleteLater();
        return Result<HttpResponse>(Error(ErrorCode::NetworkError, errorStr.toStdString()));
    }

    HttpResponse response;
    response.setStatusCode(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    response.setBody(reply->readAll());

    QList<QNetworkReply::RawHeaderPair> headers = reply->rawHeaderPairs();
    QMap<QString, QString> headerMap;
    for (const auto& pair : headers) {
        headerMap[QString::fromUtf8(pair.first)] = QString::fromUtf8(pair.second);
    }
    response.setHeaders(headerMap);

    for (const auto& interceptor : m_interceptors) {
        interceptor->onResponse(response);
    }

    reply->deleteLater();
    return Result<HttpResponse>(response);
}

void HttpClient::sendAsync(const HttpRequest& request, ResponseCallback callback) {
    HttpRequest mutableRequest = request;
    
    for (const auto& interceptor : m_interceptors) {
        interceptor->onRequest(mutableRequest);
    }

    QNetworkRequest qrequest(mutableRequest.buildUrl());

    for (const auto& header : mutableRequest.headers().toStdMap()) {
        qrequest.setRawHeader(header.first.toUtf8(), header.second.toUtf8());
    }

    QNetworkReply* reply = nullptr;
    switch (mutableRequest.method()) {
    case HttpMethod::Get:
        reply = m_manager->get(qrequest);
        break;
    case HttpMethod::Post:
        reply = m_manager->post(qrequest, mutableRequest.body());
        break;
    case HttpMethod::Put:
        reply = m_manager->put(qrequest, mutableRequest.body());
        break;
    case HttpMethod::Delete:
        reply = m_manager->deleteResource(qrequest);
        break;
    default:
        reply = m_manager->get(qrequest);
    }

    std::vector<std::shared_ptr<IInterceptor>> interceptorsCopy = m_interceptors;
    QObject::connect(reply, &QNetworkReply::finished, [reply, callback, interceptorsCopy]() {
        if (reply->error() != QNetworkReply::NoError) {
            QString errorStr = reply->errorString();
            reply->deleteLater();
            callback(Result<HttpResponse>(Error(ErrorCode::NetworkError, errorStr.toStdString())));
            return;
        }

        HttpResponse response;
        response.setStatusCode(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
        response.setBody(reply->readAll());

        QList<QNetworkReply::RawHeaderPair> headers = reply->rawHeaderPairs();
        QMap<QString, QString> headerMap;
        for (const auto& pair : headers) {
            headerMap[QString::fromUtf8(pair.first)] = QString::fromUtf8(pair.second);
        }
        response.setHeaders(headerMap);

        for (const auto& interceptor : interceptorsCopy) {
            interceptor->onResponse(response);
        }

        reply->deleteLater();
        callback(Result<HttpResponse>(response));
    });
}

void HttpClient::addInterceptor(std::shared_ptr<IInterceptor> interceptor) {
    m_interceptors.push_back(interceptor);
}

void HttpClient::removeInterceptor(IInterceptor* interceptor) {
    auto it = std::find_if(m_interceptors.begin(), m_interceptors.end(),
                           [interceptor](const std::shared_ptr<IInterceptor>& i) {
                               return i.get() == interceptor;
                           });
    if (it != m_interceptors.end()) {
        m_interceptors.erase(it);
    }
}

void HttpClient::setRetryPolicy(const network::RetryPolicy& policy) {
    m_retryPolicy = policy;
}

network::RetryPolicy HttpClient::retryPolicy() const {
    return m_retryPolicy;
}

void HttpClient::setTimeout(int ms) {
    m_timeout = ms;
}

int HttpClient::timeout() const {
    return m_timeout;
}

void HttpClient::setupSslConfiguration() {
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(config);
}

}