#include "soul/network/http_client.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QEventLoop>
#include <QTimer>
#include <QThread>

namespace sc {

HttpClient::HttpClient(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
}

HttpClient::~HttpClient() {}

namespace {

QNetworkReply* sendRequest(QNetworkAccessManager* manager, const QNetworkRequest& request,
                           HttpMethod method, const QByteArray& body) {
    switch (method) {
    case HttpMethod::Get:
        return manager->get(request);
    case HttpMethod::Post:
        return manager->post(request, body);
    case HttpMethod::Put:
        return manager->put(request, body);
    case HttpMethod::Delete:
        return manager->deleteResource(request);
    case HttpMethod::Patch:
        return manager->sendCustomRequest(request, "PATCH", body);
    case HttpMethod::Head:
        return manager->head(request);
    case HttpMethod::Options:
        return manager->sendCustomRequest(request, "OPTIONS");
    default:
        return manager->get(request);
    }
}

bool shouldRetry(QNetworkReply::NetworkError error) {
    return error == QNetworkReply::NetworkError::ConnectionRefusedError ||
           error == QNetworkReply::NetworkError::RemoteHostClosedError ||
           error == QNetworkReply::NetworkError::TimeoutError ||
           error == QNetworkReply::NetworkError::SslHandshakeFailedError ||
           error == QNetworkReply::NetworkError::TemporaryNetworkFailureError ||
           error == QNetworkReply::NetworkError::NetworkSessionFailedError;
}

}

Result<HttpResponse> HttpClient::send(const HttpRequest& request) {
    HttpRequest mutableRequest = request;

    for (const auto& interceptor : m_interceptors) {
        interceptor->onRequest(mutableRequest);
    }

    int retryCount = 0;
    const int maxRetries = m_retryPolicy.maxRetries();

    while (true) {
        QNetworkRequest qrequest(mutableRequest.buildUrl());

        for (const auto& header : mutableRequest.headers().toStdMap()) {
            qrequest.setRawHeader(header.first.toUtf8(), header.second.toUtf8());
        }

        QNetworkReply* reply = sendRequest(m_manager, qrequest, mutableRequest.method(), mutableRequest.body());

        QEventLoop loop;
        bool timedOut = false;

        QTimer::singleShot(mutableRequest.timeout(), &loop, [&loop, &timedOut, reply]() {
            timedOut = true;
            reply->abort();
            loop.quit();
        });

        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

        loop.exec();

        if (timedOut) {
            reply->deleteLater();
            if (retryCount < maxRetries && shouldRetry(QNetworkReply::TimeoutError)) {
                retryCount++;
                QThread::msleep(m_retryPolicy.nextDelay(retryCount));
                continue;
            }
            return Result<HttpResponse>(Error(ErrorCode::Timeout, "Request timed out"));
        }

        if (reply->error() != QNetworkReply::NoError) {
            QString errorStr = reply->errorString();
            auto netError = reply->error();
            reply->deleteLater();

            if (retryCount < maxRetries && shouldRetry(netError)) {
                retryCount++;
                QThread::msleep(m_retryPolicy.nextDelay(retryCount));
                continue;
            }

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
}

void HttpClient::sendAsync(const HttpRequest& request, ResponseCallback callback) {
    HttpRequest mutableRequest = request;

    for (const auto& interceptor : m_interceptors) {
        interceptor->onRequest(mutableRequest);
    }

    auto performRequest = [this, mutableRequest, callback](int retryCount = 0) {
        QNetworkRequest qrequest(mutableRequest.buildUrl());

        for (const auto& header : mutableRequest.headers().toStdMap()) {
            qrequest.setRawHeader(header.first.toUtf8(), header.second.toUtf8());
        }

        QNetworkReply* reply = sendRequest(m_manager, qrequest, mutableRequest.method(), mutableRequest.body());

        auto timedOut = std::make_shared<std::atomic<bool>>(false);
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);

        connect(timer, &QTimer::timeout, [reply, timedOut]() {
            timedOut->store(true);
            reply->abort();
        });
        timer->start(mutableRequest.timeout());

        std::vector<std::shared_ptr<network::HttpInterceptor>> interceptorsCopy = m_interceptors;
        network::RetryPolicy retryPolicyCopy = m_retryPolicy;
        int maxRetries = retryPolicyCopy.maxRetries();

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, callback, interceptorsCopy,
            retryPolicyCopy, maxRetries, retryCount, mutableRequest, timer, timedOut]() {
            timer->deleteLater();

            bool isTimedOut = timedOut->load();
            if (isTimedOut) {
                reply->deleteLater();
                if (retryCount < maxRetries && shouldRetry(QNetworkReply::TimeoutError)) {
                    QTimer::singleShot(retryPolicyCopy.nextDelay(retryCount + 1), [this, mutableRequest, callback, retryCount]() {
                        this->sendAsync(mutableRequest, callback);
                    });
                    return;
                }
                callback(Result<HttpResponse>(Error(ErrorCode::Timeout, "Request timed out")));
                return;
            }

            if (reply->error() != QNetworkReply::NoError) {
                QString errorStr = reply->errorString();
                auto netError = reply->error();
                reply->deleteLater();

                if (retryCount < maxRetries && shouldRetry(netError)) {
                    QTimer::singleShot(retryPolicyCopy.nextDelay(retryCount + 1), [this, mutableRequest, callback, retryCount]() {
                        this->sendAsync(mutableRequest, callback);
                    });
                    return;
                }

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
    };

    performRequest();
}

void HttpClient::addInterceptor(std::shared_ptr<network::HttpInterceptor> interceptor) {
    m_interceptors.push_back(interceptor);
}

void HttpClient::removeInterceptor(network::HttpInterceptor* interceptor) {
    auto it = std::find_if(m_interceptors.begin(), m_interceptors.end(),
                           [interceptor](const std::shared_ptr<network::HttpInterceptor>& i) {
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

}