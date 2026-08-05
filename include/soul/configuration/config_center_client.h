#ifndef SOUL_CONFIGURATION_CONFIG_CENTER_CLIENT_H
#define SOUL_CONFIGURATION_CONFIG_CENTER_CLIENT_H

// ============================================================================
// config_center_client.h — 配置中心统一客户端 [v2.5.0]
// ============================================================================
// 提供 Etcd/Nacos 配置中心的统一抽象层，支持:
//   - 多后端切换 (Etcd / Nacos / 本地文件)
//   - 配置监听 (Watch) 与热更新
//   - 配置缓存 (本地 + 远程)
//   - 配置版本管理
//   - 配置回滚
// ============================================================================

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QTimer>
#include <functional>
#include <memory>
#include <mutex>

#include "soul/core/result.h"
#include "soul/configuration/remote_config.h"
#include "soul/utils/json/json_helper.h"

namespace sc {

// ============================================================================
// ConfigCenterBackend — 配置中心后端枚举
// ============================================================================
enum class ConfigCenterBackend {
    Etcd,     // Etcd v3
    Nacos,    // Nacos 2.x
    Consul,   // Consul KV
    Local     // 本地文件（开发/测试）
};

// ============================================================================
// ConfigCenterConfig — 配置中心连接配置
// ============================================================================
struct ConfigCenterConfig {
    ConfigCenterBackend backend = ConfigCenterBackend::Local;
    QString endpoints = "http://127.0.0.1:2379";  // 服务器地址
    QString namespace_ = "default";                // 命名空间
    QString group = "DEFAULT_GROUP";               // 配置分组 (Nacos)
    QString dataId = "application";                // 配置 ID (Nacos)
    int timeoutMs = 10000;                         // 超时时间
    int watchIntervalMs = 30000;                   // 监听间隔
    int cacheTtlMs = 60000;                        // 本地缓存 TTL
    QString username;                              // 用户名
    QString password;                              // 密码
    QString token;                                 // 认证 Token
    bool enableSsl = false;                        // 是否启用 SSL
    QString caCertPath;                            // CA 证书路径
};

// ============================================================================
// ConfigChangeEvent — 配置变更事件
// ============================================================================
struct ConfigChangeEvent {
    QString key;                // 变更的配置键
    sc::json::Json oldValue;    // 旧值
    sc::json::Json newValue;    // 新值
    qint64 timestamp = 0;       // 变更时间戳
    QString source;             // 变更来源 (etcd/nacos/local)
};

// ============================================================================
// IConfigCenterClient — 配置中心客户端抽象接口
// ============================================================================
class IConfigCenterClient {
public:
    virtual ~IConfigCenterClient() = default;

    // === 连接管理 ===
    virtual Result<void> connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // === 配置读取 ===
    virtual Result<sc::json::Json> getConfig(const QString& key) = 0;
    virtual Result<sc::json::Json> getConfig(const QString& key,
                                              const sc::json::Json& defaultValue) = 0;

    // === 配置写入 ===
    virtual Result<void> setConfig(const QString& key, const sc::json::Json& value) = 0;
    virtual Result<void> deleteConfig(const QString& key) = 0;

    // === 批量操作 ===
    virtual Result<sc::json::Json> getConfigs(const QString& prefix = "") = 0;

    // === 配置监听 ===
    using ConfigChangeCallback = std::function<void(const ConfigChangeEvent&)>;
    virtual Result<void> watch(const QString& key, ConfigChangeCallback callback) = 0;
    virtual Result<void> unwatch(const QString& key) = 0;

    // === 配置版本 ===
    virtual Result<qint64> getVersion(const QString& key) = 0;

    // === 配置回滚 ===
    virtual Result<void> rollback(const QString& key, qint64 version) = 0;

    // === 后端信息 ===
    virtual ConfigCenterBackend backend() const = 0;
};

// ============================================================================
// ConfigCenterClient — 配置中心统一客户端
// ============================================================================
class ConfigCenterClient : public QObject, public IConfigCenterClient {
    Q_OBJECT
public:
    static ConfigCenterClient& instance();

    // === 初始化 ===
    Result<void> initialize(const ConfigCenterConfig& config);
    void shutdown();

    // === IConfigCenterClient 实现 ===
    Result<void> connect() override;
    void disconnect() override;
    bool isConnected() const override;

    Result<sc::json::Json> getConfig(const QString& key) override;
    Result<sc::json::Json> getConfig(const QString& key,
                                      const sc::json::Json& defaultValue) override;
    Result<void> setConfig(const QString& key, const sc::json::Json& value) override;
    Result<void> deleteConfig(const QString& key) override;
    Result<sc::json::Json> getConfigs(const QString& prefix = "") override;
    Result<void> watch(const QString& key, ConfigChangeCallback callback) override;
    Result<void> unwatch(const QString& key) override;
    Result<qint64> getVersion(const QString& key) override;
    Result<void> rollback(const QString& key, qint64 version) override;
    ConfigCenterBackend backend() const override;

    // === 属性优先级合并 ===
    // 合并远程配置到本地配置（远程覆盖本地）
    sc::json::Json mergeConfig(const sc::json::Json& local,
                                const sc::json::Json& remote) const;

signals:
    void connected();
    void disconnected();
    void configChanged(const ConfigChangeEvent& event);
    void connectionError(const QString& error);

private:
    ConfigCenterClient() = default;
    ~ConfigCenterClient() override;

    void startWatchTimer();
    void stopWatchTimer();
    void onWatchTimerTick();

    void notifyChange(const QString& key, const sc::json::Json& oldVal,
                      const sc::json::Json& newVal);

    ConfigCenterConfig m_config;
    std::unique_ptr<IRemoteConfigSource> m_backend;
    QTimer* m_watchTimer = nullptr;
    sc::json::Json m_cache;        // 本地配置缓存
    QHash<QString, qint64> m_versions;  // 配置版本号
    QHash<QString, QList<ConfigChangeCallback>> m_watchers;
    mutable std::mutex m_mutex;
    bool m_connected = false;
};

} // namespace sc

#endif // SOUL_CONFIGURATION_CONFIG_CENTER_CLIENT_H