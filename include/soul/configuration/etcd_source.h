#ifndef SOUL_CONFIGURATION_ETCD_SOURCE_H
#define SOUL_CONFIGURATION_ETCD_SOURCE_H

#include "remote_config.h"
#include "soul/utils/json/json_helper.h"
#include <QNetworkAccessManager>
#include <QTimer>
#include <functional>

namespace sc {

/**
 * @class EtcdConfigSource
 * @brief Etcd 配置中心适配器
 *
 * EtcdConfigSource 通过 Etcd v3 HTTP API（gRPC-gateway）与 Etcd 集群交互，
 * 支持配置的拉取、发布和监听（Watch）。
 *
 * Etcd v3 API 参考：
 * - 获取配置：POST /v3/kv/range
 * - 发布配置：POST /v3/kv/put
 * - 监听配置：POST /v3/kv/watch
 *
 * 使用方式：
 * @code
 * EtcdConfigSource source("http://127.0.0.1:2379");
 * source.connectToServer();
 * auto config = source.fetchConfig("myapp-config");
 * source.watchConfig("myapp-config", [](const sc::json::Json& cfg) {
 *     // 处理配置变更
 * });
 * @endcode
 *
 * @see IRemoteConfigSource, RemoteConfiguration
 */
class EtcdConfigSource : public IRemoteConfigSource {
public:
    /**
     * @brief 构造函数
     * @param endpoints Etcd 端点地址（如 "http://127.0.0.1:2379"）
     */
    explicit EtcdConfigSource(const QString& endpoints = "http://127.0.0.1:2379");

    /**
     * @brief 析构函数
     */
    ~EtcdConfigSource() override;

    /** @copydoc IRemoteConfigSource::connectToServer */
    Result<void> connectToServer() override;

    /** @copydoc IRemoteConfigSource::fetchConfig */
    Result<sc::json::Json> fetchConfig(const QString& namespaceName) override;

    /** @copydoc IRemoteConfigSource::watchConfig */
    Result<void> watchConfig(const QString& namespaceName,
                             std::function<void(const sc::json::Json&)> callback) override;

    /** @copydoc IRemoteConfigSource::publishConfig */
    Result<void> publishConfig(const QString& namespaceName,
                               const sc::json::Json& config) override;

    /** @copydoc IRemoteConfigSource::disconnectFromServer */
    void disconnectFromServer() override;

private:
    QByteArray syncPost(const QUrl& url, const QByteArray& body, int timeoutMs = 10000);
    QString encodeBase64(const QByteArray& data) const;
    QByteArray decodeBase64(const QString& data) const;

    QString m_endpoints;
    QNetworkAccessManager m_networkManager;
    QTimer* m_watchTimer = nullptr;
    std::function<void(const sc::json::Json&)> m_watchCallback;
    QString m_watchNamespace;
};

} // namespace sc

#endif // SOUL_CONFIGURATION_ETCD_SOURCE_H