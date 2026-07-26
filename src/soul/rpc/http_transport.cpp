#include "soul/rpc/http_transport.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QJsonArray>
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

    // URL 编码 serviceName/methodName,防止路径/查询注入
    // 例如 serviceName="a/b?c=d" 会被编码为 "a%2Fb%3Fc%3Dd",不会破坏 URL 结构
    const QString encodedService = QString::fromUtf8(
        QUrl::toPercentEncoding(request.serviceName));
    const QString encodedMethod = QString::fromUtf8(
        QUrl::toPercentEncoding(request.methodName));
    QUrl url(m_baseUrl + "/rpc/" + encodedService + "/" + encodedMethod);
    QNetworkRequest qrequest(url);
    qrequest.setHeader(QNetworkRequest::ContentTypeHeader, m_serializer->contentType());

    for (auto it = m_headers.constBegin(); it != m_headers.constEnd(); ++it) {
        qrequest.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    // 构造 RPC envelope:包含 service/method/requestId/params
    // requestId 必须在请求体中,服务端才能将响应关联到请求
    QJsonObject envelope;
    envelope["service"] = request.serviceName;
    envelope["method"] = request.methodName;
    envelope["requestId"] = request.requestId;
    // params 由序列化器输出 JSON,解析后嵌入 envelope(保持序列化器抽象)
    const QByteArray paramsJson = m_serializer->serialize(request.params);
    const QJsonDocument paramsDoc = QJsonDocument::fromJson(paramsJson);
    if (paramsDoc.isObject()) {
        envelope["params"] = paramsDoc.object();
    } else if (paramsDoc.isArray()) {
        envelope["params"] = paramsDoc.array();
    } else {
        // params 不是对象/数组(可能是基础类型),作为 QJsonValue 直接嵌入
        envelope["params"] = QJsonValue(QString::fromUtf8(paramsJson));
    }
    const QByteArray body = QJsonDocument(envelope).toJson(QJsonDocument::Compact);

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

void HttpTransport::setReadTimeout(int ms) {
    m_readTimeout = ms;
}

} // namespace rpc
} // namespace sc