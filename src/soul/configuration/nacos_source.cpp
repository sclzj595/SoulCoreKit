#include "soul/configuration/nacos_source.h"
#include "soul/core/error.h"
#include "soul/utils/json/json_helper.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrlQuery>
#include <QTimer>

namespace sc {

NacosConfigSource::NacosConfigSource(const QString& serverAddr, const QString& group)
    : m_serverAddr(serverAddr)
    , m_group(group)
{
}

NacosConfigSource::~NacosConfigSource() {
    disconnectFromServer();
}

Result<void> NacosConfigSource::connectToServer() {
    // 通过尝试获取一个不存在的配置来验证服务器可达性
    QUrl url(m_serverAddr + "/nacos/v1/cs/configs");
    QUrlQuery query;
    query.addQueryItem("dataId", "__health_check__");
    query.addQueryItem("group", m_group);
    url.setQuery(query);

    QByteArray response = syncGet(url, 5000);
    return Ok();
}

Result<sc::json::Json> NacosConfigSource::fetchConfig(const QString& namespaceName) {
    QUrl url(m_serverAddr + "/nacos/v1/cs/configs");
    QUrlQuery query;
    query.addQueryItem("dataId", namespaceName);
    query.addQueryItem("group", m_group);
    url.setQuery(query);

    QByteArray data = syncGet(url, 10000);
    if (data.isEmpty()) {
        return Error(ErrorCode::NotFound,
                     "Nacos config not found: " + namespaceName.toStdString());
    }

    auto result = sc::json::deserialize(data);
    if (!result.isOk()) {
        return Error(ErrorCode::ParseError,
                     "Nacos config is not valid JSON for: " + namespaceName.toStdString());
    }

    sc::json::Json j = result.unwrap();
    if (!j.is_object()) {
        return Error(ErrorCode::ParseError,
                     "Nacos config is not valid JSON for: " + namespaceName.toStdString());
    }
    return Result<sc::json::Json>(j);
}

Result<void> NacosConfigSource::watchConfig(const QString& namespaceName,
                                            std::function<void(const sc::json::Json&)> callback) {
    // 停止之前的监听
    if (m_watchTimer) {
        m_watchTimer->stop();
        m_watchTimer->deleteLater();
        m_watchTimer = nullptr;
    }

    m_watchCallback = std::move(callback);
    m_watchNamespace = namespaceName;

    // 使用定时轮询实现长轮询效果
    m_watchTimer = new QTimer();
    QObject::connect(m_watchTimer, &QTimer::timeout, [this]() {
        if (!m_watchCallback) return;
        auto result = fetchConfig(m_watchNamespace);
        if (result.isOk()) {
            m_watchCallback(result.unwrap());
        }
    });
    m_watchTimer->start(30000); // 每 30 秒轮询一次

    return Ok();
}

Result<void> NacosConfigSource::publishConfig(const QString& namespaceName,
                                              const sc::json::Json& config) {
    QUrl url(m_serverAddr + "/nacos/v1/cs/configs");

    QUrlQuery postData;
    postData.addQueryItem("dataId", namespaceName);
    postData.addQueryItem("group", m_group);
    postData.addQueryItem("content",
                          QString::fromUtf8(sc::json::serialize(config)));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    QNetworkReply* reply = m_networkManager.post(
        request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg = reply->errorString();
        reply->deleteLater();
        return Error(ErrorCode::NetworkError,
                     "Nacos publish failed: " + errMsg.toStdString());
    }

    QByteArray response = reply->readAll();
    reply->deleteLater();

    if (response.trimmed() != "true") {
        return Error(ErrorCode::InternalError,
                     "Nacos publish returned non-true response");
    }

    return Ok();
}

void NacosConfigSource::disconnectFromServer() {
    if (m_watchTimer) {
        m_watchTimer->stop();
        m_watchTimer->deleteLater();
        m_watchTimer = nullptr;
    }
    m_watchCallback = nullptr;
    m_watchNamespace.clear();
}

QByteArray NacosConfigSource::syncGet(const QUrl& url, int timeoutMs) {
    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager.get(request);

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

} // namespace sc