#include "soul/rpc/http_transport.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QPointer>
#include <QHttp2Configuration>

#include "soul/utils/json/json_helper.h"

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
    if (!m_running.load(std::memory_order_acquire) || !m_manager) {
        return Result<RpcResponse>(Error(ErrorCode::NetworkError, "Transport not running"));
    }

    const QString encodedService = QString::fromUtf8(
        QUrl::toPercentEncoding(request.serviceName));
    const QString encodedMethod = QString::fromUtf8(
        QUrl::toPercentEncoding(request.methodName));
    QUrl url(m_baseUrl + "/rpc/" + encodedService + "/" + encodedMethod);
    QNetworkRequest qrequest(url);
    qrequest.setHeader(QNetworkRequest::ContentTypeHeader, m_serializer->contentType());

    const bool http2Enabled = m_http2Enabled.load(std::memory_order_acquire);
    qrequest.setAttribute(QNetworkRequest::Http2AllowedAttribute, http2Enabled);
    if (http2Enabled) {
        QHttp2Configuration http2Config = qrequest.http2Configuration();
        http2Config.setServerPushEnabled(false);
        qrequest.setHttp2Configuration(http2Config);
    }

    for (auto it = m_headers.constBegin(); it != m_headers.constEnd(); ++it) {
        qrequest.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    // 构造 RPC envelope (使用 nlohmann/json)
    sc::json::Json envelope = sc::json::Json::object();
    envelope["service"] = request.serviceName.toStdString();
    envelope["method"] = request.methodName.toStdString();
    envelope["requestId"] = request.requestId.toStdString();
    envelope["params"] = request.params;

    const QByteArray body = sc::json::serialize(envelope);

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

    auto parseResult = sc::json::deserialize(responseBody);
    if (parseResult.isErr()) {
        return Result<RpcResponse>(Error(ErrorCode::DeserializationError,
            parseResult.unwrapErr().message()));
    }

    sc::json::Json responseObj = parseResult.unwrap();
    if (!responseObj.is_object()) {
        return Result<RpcResponse>(Error(ErrorCode::DeserializationError, "Invalid response format"));
    }

    RpcResponse rpcResponse;
    rpcResponse.success = sc::json::getBool(responseObj, "success", false);
    rpcResponse.data = sc::json::getObject(responseObj, "data");
    rpcResponse.errorMessage = sc::json::getString(responseObj, "errorMessage");
    rpcResponse.requestId = sc::json::getString(responseObj, "requestId");

    return Result<RpcResponse>(rpcResponse);
}

void HttpTransport::start() {
    if (m_running.exchange(true)) return;
    m_manager = new QNetworkAccessManager(this);
}

void HttpTransport::stop() {
    if (!m_running.exchange(false)) return;
    if (m_manager) {
        m_manager->deleteLater();
        m_manager = nullptr;
    }
}

bool HttpTransport::isRunning() const {
    return m_running.load(std::memory_order_acquire);
}

void HttpTransport::setSerializer(std::shared_ptr<ISerializer> serializer) {
    m_serializer = std::move(serializer);
}

std::shared_ptr<ISerializer> HttpTransport::getSerializer() const {
    return m_serializer;
}

void HttpTransport::setReadTimeout(int ms) {
    m_readTimeout = ms;
}

void HttpTransport::setHttp2Enabled(bool enabled) {
    m_http2Enabled.store(enabled, std::memory_order_release);
}

bool HttpTransport::isHttp2Enabled() const {
    return m_http2Enabled.load(std::memory_order_acquire);
}

} // namespace rpc
} // namespace sc