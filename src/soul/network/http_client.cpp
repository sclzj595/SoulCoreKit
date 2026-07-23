#include "soul/network/http_client.h"
#include <memory>
#include <atomic>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QEventLoop>
#include <QTimer>
#include <QThread>
#include "soul/logging/logger.h"

namespace sc {
namespace network {

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

HttpResponse buildResponse(QNetworkReply* reply) {
    HttpResponse response;
    response.setStatusCode(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    response.setBody(reply->readAll());

    QList<QNetworkReply::RawHeaderPair> headers = reply->rawHeaderPairs();
    QMap<QString, QString> headerMap;
    for (const auto& pair : headers) {
        headerMap[QString::fromUtf8(pair.first)] = QString::fromUtf8(pair.second);
    }
    response.setHeaders(headerMap);

    return response;
}

}

HttpClient::HttpClient(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
    setupSslConfiguration();
}

HttpClient::~HttpClient() {}

void HttpClient::setupSslConfiguration() {
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2OrLater);
    config.setPeerVerifyMode(QSslSocket::VerifyPeer);
    config.setPeerVerifyDepth(4);
    QSslConfiguration::setDefaultConfiguration(config);
}

Result<HttpResponse> HttpClient::send(const HttpRequest& request) {
#ifdef QT_DEBUG
    auto app = QCoreApplication::instance();
    if (app && QThread::currentThread() == app->thread()) {
        Logger::instance().warn("HttpClient::send() is called in GUI thread, consider using sendAsync()", "HttpClient");
    }
#endif
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

        HttpResponse response = buildResponse(reply);

        for (const auto& interceptor : m_interceptors) {
            interceptor->onResponse(response);
        }

        reply->deleteLater();
        return Result<HttpResponse>(response);
    }
}

void HttpClient::sendAsync(const HttpRequest& request, ResponseCallback callback) {
    sendAsync(request, callback, 0);
}

void HttpClient::sendAsync(const HttpRequest& request, ResponseCallback callback, int retryCount) {
    HttpRequest mutableRequest = request;

    for (const auto& interceptor : m_interceptors) {
        interceptor->onRequest(mutableRequest);
    }

    std::vector<std::shared_ptr<HttpInterceptor>> interceptorsCopy = m_interceptors;
    RetryPolicy retryPolicyCopy = m_retryPolicy;
    int maxRetries = retryPolicyCopy.maxRetries();

    auto performRequest = [this, mutableRequest, callback, interceptorsCopy,
                          retryPolicyCopy, maxRetries, retryCount]() {
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

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, callback, interceptorsCopy,
            retryPolicyCopy, maxRetries, retryCount, mutableRequest, timer, timedOut]() {
            timer->deleteLater();

            bool isTimedOut = timedOut->load();
            if (isTimedOut) {
                reply->deleteLater();
                if (retryCount < maxRetries && shouldRetry(QNetworkReply::TimeoutError)) {
                    QTimer::singleShot(retryPolicyCopy.nextDelay(retryCount + 1), [this, mutableRequest, callback, retryCount]() {
                        this->sendAsync(mutableRequest, callback, retryCount + 1);
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
                        this->sendAsync(mutableRequest, callback, retryCount + 1);
                    });
                    return;
                }

                callback(Result<HttpResponse>(Error(ErrorCode::NetworkError, errorStr.toStdString())));
                return;
            }

            HttpResponse response = buildResponse(reply);

            for (const auto& interceptor : interceptorsCopy) {
                interceptor->onResponse(response);
            }

            reply->deleteLater();
            callback(Result<HttpResponse>(response));
        });
    };

    performRequest();
}

void HttpClient::addInterceptor(std::shared_ptr<HttpInterceptor> interceptor) {
    m_interceptors.push_back(interceptor);
}

void HttpClient::removeInterceptor(HttpInterceptor* interceptor) {
    auto it = std::find_if(m_interceptors.begin(), m_interceptors.end(),
                           [interceptor](const std::shared_ptr<HttpInterceptor>& i) {
                               return i.get() == interceptor;
                           });
    if (it != m_interceptors.end()) {
        m_interceptors.erase(it);
    }
}

void HttpClient::setRetryPolicy(const RetryPolicy& policy) {
    m_retryPolicy = policy;
}

RetryPolicy HttpClient::retryPolicy() const {
    return m_retryPolicy;
}

void HttpClient::setTimeout(int ms) {
    m_timeout = ms;
}

int HttpClient::timeout() const {
    return m_timeout;
}

} // namespace network
} // namespace sc