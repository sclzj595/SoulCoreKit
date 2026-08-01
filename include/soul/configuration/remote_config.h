#ifndef SOUL_CONFIGURATION_REMOTE_CONFIG_H
#define SOUL_CONFIGURATION_REMOTE_CONFIG_H

#include "iconfiguration.h"
#include <QObject>
#include "soul/utils/json/json_helper.h"
#include <QString>
#include <functional>
#include <memory>
#include <mutex>

namespace sc {

/**
 * @class IRemoteConfigSource
 * @brief 远程配置源抽象接口
 *
 * IRemoteConfigSource 定义了与远程配置中心（Nacos/Apollo/etcd）交互的标准接口。
 * 支持配置的拉取、发布、变更监听（长轮询）等操作。
 *
 * 具体实现包括：
 * - NacosConfigSource：Nacos 配置中心适配器
 * - EtcdConfigSource：Etcd 配置中心适配器
 *
 * @see RemoteConfiguration, NacosConfigSource, EtcdConfigSource
 */
class IRemoteConfigSource {
public:
    virtual ~IRemoteConfigSource() = default;

    /**
     * @brief 连接远程配置中心
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> connectToServer() = 0;

    /**
     * @brief 从远程拉取命名空间配置
     * @param namespaceName 命名空间名称（对应 Nacos dataId 或 etcd key prefix）
     * @return Result<sc::json::Json>，成功返回配置 JSON，失败返回 Error
     */
    virtual Result<sc::json::Json> fetchConfig(const QString& namespaceName) = 0;

    /**
     * @brief 监听配置变更（长轮询）
     * @param namespaceName 命名空间名称
     * @param callback 配置变更回调，参数为完整的配置 JSON
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> watchConfig(const QString& namespaceName,
                                     std::function<void(const sc::json::Json&)> callback) = 0;

    /**
     * @brief 发布配置到远程
     * @param namespaceName 命名空间名称
     * @param config 配置 JSON 对象
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> publishConfig(const QString& namespaceName,
                                       const sc::json::Json& config) = 0;

    /**
     * @brief 断开与远程配置中心的连接
     */
    virtual void disconnectFromServer() = 0;
};

/**
 * @class RemoteConfiguration
 * @brief 远程配置实现，扩展 IConfiguration 接口支持远程配置中心
 *
 * RemoteConfiguration 在 IConfiguration 基础上增加了远程配置源支持，
 * 通过 setSource() 设置具体的远程配置源（如 Nacos/Etcd），
 * 通过 sync() 从远程同步配置，通过 watch() 建立长轮询监听。
 *
 * 使用方式：
 * @code
 * auto config = std::make_shared<RemoteConfiguration>();
 * config->setSource(std::make_unique<NacosConfigSource>("http://127.0.0.1:8848"));
 * config->sync("myapp-config");
 * QString host = config->getString("server.host", "localhost");
 * config->watch("myapp-config");  // 启动长轮询监听
 * @endcode
 *
 * @see IConfiguration, IRemoteConfigSource, NacosConfigSource
 */
class RemoteConfiguration : public QObject, public IConfiguration {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit RemoteConfiguration(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~RemoteConfiguration() override = default;

    /**
     * @brief 设置远程配置源
     * @param source 远程配置源实例（移交所有权）
     */
    void setSource(std::unique_ptr<IRemoteConfigSource> source);

    /**
     * @brief 同步远程配置
     * @param namespaceName 命名空间名称
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    Result<void> sync(const QString& namespaceName);

    /**
     * @brief 启动配置变更监听（长轮询）
     * @param namespaceName 命名空间名称
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    Result<void> watch(const QString& namespaceName);

    // --- IConfiguration 接口实现 ---

    /** @copydoc IConfiguration::load */
    Result<void> load(const QString& filePath) override;

    /** @copydoc IConfiguration::save */
    Result<void> save(const QString& filePath) override;

    /** @copydoc IConfiguration::getString */
    QString getString(const QString& key, const QString& defaultValue = "") const override;

    /** @copydoc IConfiguration::getInt */
    int getInt(const QString& key, int defaultValue = 0) const override;

    /** @copydoc IConfiguration::getDouble */
    double getDouble(const QString& key, double defaultValue = 0.0) const override;

    /** @copydoc IConfiguration::getBool */
    bool getBool(const QString& key, bool defaultValue = false) const override;

    /** @copydoc IConfiguration::setString */
    void setString(const QString& key, const QString& value) override;

    /** @copydoc IConfiguration::setInt */
    void setInt(const QString& key, int value) override;

    /** @copydoc IConfiguration::setDouble */
    void setDouble(const QString& key, double value) override;

    /** @copydoc IConfiguration::setBool */
    void setBool(const QString& key, bool value) override;

    /** @copydoc IConfiguration::contains */
    bool contains(const QString& key) const override;

    /** @copydoc IConfiguration::remove */
    void remove(const QString& key) override;

signals:
    /**
     * @brief 配置变更信号
     * @param key 变更的配置键
     * @param value 变更后的配置值
     */
    void configChanged(const QString& key, const QString& value);

    /**
     * @brief 配置同步完成信号
     * @param namespaceName 同步的命名空间名称
     */
    void configSynced(const QString& namespaceName);

private:
    sc::json::Json m_config;
    std::unique_ptr<IRemoteConfigSource> m_source;
    mutable std::mutex m_mutex;
};

} // namespace sc

#endif // SOUL_CONFIGURATION_REMOTE_CONFIG_H