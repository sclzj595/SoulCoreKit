#include "soul/rpc/http_transport.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QPointer>

namespace sc {
namespace rpc {

HttpTransport::HttpTransport(const QString& baseUrl, const QMap<QString, QString>& headers, QObject* parent)
    : QObject(parent)
    , m_baseUrl(baseUrl)
    , m_headers(headers)
    , m_manager(nullptr)
    , m_serializer(std::make_shared<JsonSerializer>())
{
}

HttpTransport::~HttpTransport() {
    stop();
}

Result<RpcResponse> HttpTransport::sendRequest(const RpcRequest& request) {
    if (!m_running || !m_manager) {
        return Result<RpcResponse>(Error(ErrorCode::NetworkError, "Transport not running"));
    }

    QUrl url(m_baseUrl + "/rpc/" + request.serviceName + "/" + request.methodName);
    QNetworkRequest qrequest(url);
    qrequest.setHeader(QNetworkRequest::ContentTypeHeader, m_serializer->contentType());

    for (auto it = m_headers.constBegin(); it != m_headers.constEnd(); ++it) {
        qrequest.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QByteArray body = m_serializer->serialize(request.params);

    QNetworkReply* reply = m_manager->post(qrequest, body);

    QEventLoop loop;
    bool timedOut = false;

    QTimer timer;
    timer.setSingleShot(true);
    QPointer<QNetworkReply> replyGuard(reply);
    QObject::connect(&timer, &QTimer::timeout, [&loop, &timedOut, replyGuard]() {
        timedOut = true;
        if (replyGuard) replyGuard->abort();
        loop.quit();
    });
    timer.start(m_readTimeout);

    QObject::connect(reply, &QNetworkReply::finished, [&loop, &timer]() {
        timer.stop();
        loop.quit();
    });

    loop.exec();

    if (timedOut) {
        reply->deleteLater();
        return Result<RpcResponse>(Error(ErrorCode::Timeout, "HTTP request timed out"));
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString errorStr = reply->errorString();
        reply->deleteLater();
        return Result<RpcResponse>(Error(ErrorCode::NetworkError, errorStr));
    }

    QByteArray responseBody = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseBody, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return Result<RpcResponse>(Error(ErrorCode::DeserializationError, parseError.errorString()));
    }

    if (!doc.isObject()) {
        return Result<RpcResponse>(Error(ErrorCode::DeserializationError, "Invalid response format"));
    }

    QJsonObject responseObj = doc.object();
    RpcResponse rpcResponse;
    rpcResponse.success = responseObj.value("success").toBool(false);
    rpcResponse.data = responseObj.value("data").toObject();
    rpcResponse.errorMessage = responseObj.value("errorMessage").toString();
    rpcResponse.requestId = responseObj.value("requestId").toString();

    return Result<RpcResponse>(rpcResponse);
}

void HttpTransport::start() {
    if (m_running) return;
    m_manager = new QNetworkAccessManager(this);
    m_running = true;
}

void HttpTransport::stop() {
    if (!m_running) return;
    if (m_manager) {
        m_manager->deleteLater();
        m_manager = nullptr;
    }
    m_running = false;
}

bool HttpTransport::isRunning() const {
    return m_running;
}

void HttpTransport::setSerializer(std::shared_ptr<ISerializer> serializer) {
    m_serializer = std::move(serializer);
}

std::shared_ptr<ISerializer> HttpTransport::getSerializer() const {
    return m_serializer;
}

void HttpTransport::setConnectTimeout(int ms) {
    m_connectTimeout = ms;
}

void HttpTransport::setReadTimeout(int ms) {
    m_readTimeout = ms;
}

} // namespace rpc
} // namespace sc