#include "soul/configuration/etcd_source.h"
#include "soul/core/error.h"
#include "soul/utils/json/json_helper.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>

namespace sc {

EtcdConfigSource::EtcdConfigSource(const QString& endpoints)
    : m_endpoints(endpoints)
{
}

EtcdConfigSource::~EtcdConfigSource() {
    disconnectFromServer();
}

Result<void> EtcdConfigSource::connectToServer() {
    // Etcd v3 健康检查: GET /v3/cluster/member/list
    QUrl url(m_endpoints + "/v3/cluster/member/list");
    sc::json::Json body = sc::json::Json::object();
    QByteArray response = syncPost(url, sc::json::serialize(body), 5000);
    if (response.isEmpty()) {
        return Error(ErrorCode::ConnectionRefused,
                     "Etcd server unreachable: " + m_endpoints.toStdString());
    }
    return Ok();
}

Result<sc::json::Json> EtcdConfigSource::fetchConfig(const QString& namespaceName) {
    // Etcd v3 range API
    QUrl url(m_endpoints + "/v3/kv/range");

    // 构建 range 请求: key 需要 base64 编码
    QString key = "/config/" + namespaceName + "/";
    QString encodedKey = encodeBase64(key.toUtf8());

    sc::json::Json requestBody = sc::json::Json::object();
    requestBody["key"] = encodedKey.toStdString();
    requestBody["range_end"] = encodeBase64(
        (key.toUtf8().left(key.length() - 1) + QByteArray(1, '\0')).left(key.length())).toStdString();

    QByteArray response = syncPost(url, sc::json::serialize(requestBody), 10000);
    if (response.isEmpty()) {
        return Error(ErrorCode::NotFound,
                     "Etcd config not found: " + namespaceName.toStdString());
    }

    auto result = sc::json::deserialize(response);
    if (!result.isOk()) {
        return Error(ErrorCode::ParseError,
                     "Etcd response parse error for: " + namespaceName.toStdString());
    }
    sc::json::Json respObj = result.unwrap();
    if (!respObj.is_object()) {
        return Error(ErrorCode::ParseError,
                     "Etcd response is not a JSON object for: " + namespaceName.toStdString());
    }

    // 解析 etcd 响应: {"kvs": [{"key": "...", "value": "..."}]}
    sc::json::Json kvs = sc::json::getArray(respObj, "kvs");
    if (kvs.empty()) {
        return Error(ErrorCode::NotFound,
                     "Etcd config not found: " + namespaceName.toStdString());
    }

    // 取第一个 kv 的值
    sc::json::Json firstKv = kvs[0];
    if (!firstKv.is_object()) {
        return Error(ErrorCode::ParseError, "Etcd config kv entry is not a JSON object");
    }

    QByteArray valueData = decodeBase64(sc::json::getString(firstKv, "value"));

    auto valueResult = sc::json::deserialize(valueData);
    if (!valueResult.isOk()) {
        return Error(ErrorCode::ParseError, "Etcd config value is not valid JSON");
    }
    sc::json::Json valueJson = valueResult.unwrap();
    if (!valueJson.is_object()) {
        return Error(ErrorCode::ParseError, "Etcd config value is not valid JSON");
    }

    return Result<sc::json::Json>(valueJson);
}

Result<void> EtcdConfigSource::watchConfig(const QString& namespaceName,
                                           std::function<void(const sc::json::Json&)> callback) {
    if (m_watchTimer) {
        m_watchTimer->stop();
        m_watchTimer->deleteLater();
        m_watchTimer = nullptr;
    }

    m_watchCallback = std::move(callback);
    m_watchNamespace = namespaceName;

    // 使用定时轮询模拟 Etcd watch
    m_watchTimer = new QTimer();
    QObject::connect(m_watchTimer, &QTimer::timeout, [this]() {
        if (!m_watchCallback) return;
        auto result = fetchConfig(m_watchNamespace);
        if (result.isOk()) {
            m_watchCallback(result.unwrap());
        }
    });
    m_watchTimer->start(30000);

    return Ok();
}

Result<void> EtcdConfigSource::publishConfig(const QString& namespaceName,
                                             const sc::json::Json& config) {
    QUrl url(m_endpoints + "/v3/kv/put");

    QString key = "/config/" + namespaceName;
    QString encodedKey = encodeBase64(key.toUtf8());
    QString encodedValue = encodeBase64(sc::json::serialize(config));

    sc::json::Json requestBody = sc::json::Json::object();
    requestBody["key"] = encodedKey.toStdString();
    requestBody["value"] = encodedValue.toStdString();

    QByteArray response = syncPost(url, sc::json::serialize(requestBody), 10000);
    if (response.isEmpty()) {
        return Error(ErrorCode::NetworkError,
                     "Etcd publish failed: " + m_endpoints.toStdString());
    }

    return Ok();
}

void EtcdConfigSource::disconnectFromServer() {
    if (m_watchTimer) {
        m_watchTimer->stop();
        m_watchTimer->deleteLater();
        m_watchTimer = nullptr;
    }
    m_watchCallback = nullptr;
    m_watchNamespace.clear();
}

QByteArray EtcdConfigSource::syncPost(const QUrl& url, const QByteArray& body, int timeoutMs) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager.post(request, body);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray data;
    if (reply->error() == QNetworkReply::NoError) {
        data = reply->readAll();
    }
    reply->deleteLater();
    return data;
}

QString EtcdConfigSource::encodeBase64(const QByteArray& data) const {
    return QString::fromUtf8(data.toBase64());
}

QByteArray EtcdConfigSource::decodeBase64(const QString& data) const {
    return QByteArray::fromBase64(data.toUtf8());
}

} // namespace sc