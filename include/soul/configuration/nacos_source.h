#ifndef SOUL_CONFIGURATION_NACOS_SOURCE_H
#define SOUL_CONFIGURATION_NACOS_SOURCE_H

#include "remote_config.h"
#include "soul/utils/json/json_helper.h"
#include <QNetworkAccessManager>
#include <QTimer>
#include <functional>

namespace sc {

/**
 * @class NacosConfigSource
 * @brief Nacos 配置中心适配器
 *
 * NacosConfigSource 通过 Nacos HTTP Open API（v1/cs/configs）与
 * Nacos 配置中心交互，支持配置拉取、发布和长轮询监听。
 *
 * Nacos API 参考：
 * - 获取配置：GET /nacos/v1/cs/configs?dataId=xxx&group=xxx
 * - 发布配置：POST /nacos/v1/cs/configs
 * - 监听配置：POST /nacos/v1/cs/configs/listener（长轮询）
 *
 * 使用方式：
 * @code
 * NacosConfigSource source("http://127.0.0.1:8848", "DEFAULT_GROUP");
 * source.connectToServer();
 * auto config = source.fetchConfig("myapp-config");
 * source.watchConfig("myapp-config", [](const sc::json::Json& cfg) {
 *     // 处理配置变更
 * });
 * @endcode
 *
 * @see IRemoteConfigSource, RemoteConfiguration
 */
class NacosConfigSource : public IRemoteConfigSource {
public:
    /**
     * @brief 构造函数
     * @param serverAddr Nacos 服务器地址（如 "http://127.0.0.1:8848"）
     * @param group 配置分组（默认 "DEFAULT_GROUP"）
     */
    explicit NacosConfigSource(const QString& serverAddr = "http://127.0.0.1:8848",
                               const QString& group = "DEFAULT_GROUP");

    /**
     * @brief 析构函数
     */
    ~NacosConfigSource() override;

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
    /**
     * @brief 执行同步 GET 请求
     * @param url 请求 URL
     * @param timeoutMs 超时时间（毫秒）
     * @return 响应数据
     */
    QByteArray syncGet(const QUrl& url, int timeoutMs = 10000);

    QString m_serverAddr;
    QString m_group;
    QNetworkAccessManager m_networkManager;
    QTimer* m_watchTimer = nullptr;
    std::function<void(const sc::json::Json&)> m_watchCallback;
    QString m_watchNamespace;
};

} // namespace sc

#endif // SOUL_CONFIGURATION_NACOS_SOURCE_H