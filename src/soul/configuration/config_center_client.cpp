// ============================================================================
// config_center_client.cpp — 配置中心统一客户端实现 [v2.5.0]
// ============================================================================

#include "soul/configuration/config_center_client.h"
#include "soul/core/error.h"

#include <QDebug>

namespace sc {

// ============================================================================
// ConfigCenterClient 构造与析构
// ============================================================================

ConfigCenterClient::~ConfigCenterClient() {
    shutdown();
}

ConfigCenterClient& ConfigCenterClient::instance() {
    static ConfigCenterClient s_instance;
    return s_instance;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> ConfigCenterClient::initialize(const ConfigCenterConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_cache = sc::json::Json::object();
    m_connected = false;
    return Result<void>::ok();
}

void ConfigCenterClient::shutdown() {
    stopWatchTimer();
    disconnect();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_backend.reset();
    m_watchers.clear();
    m_versions.clear();
    m_cache = sc::json::Json::object();
}

// ============================================================================
// 连接管理
// ============================================================================

Result<void> ConfigCenterClient::connect() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_connected) {
        return Result<void>::ok();
    }

    if (m_backend) {
        auto result = m_backend->connectToServer();
        if (result.isOk()) {
            m_connected = true;
            emit connected();
        }
        return result;
    }

    // 本地模式直接标记为已连接
    m_connected = true;
    emit connected();
    return Result<void>::ok();
}

void ConfigCenterClient::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_backend) {
        m_backend->disconnectFromServer();
    }
    m_connected = false;
    emit disconnected();
}

bool ConfigCenterClient::isConnected() const {
    return m_connected;
}

// ============================================================================
// 配置读取
// ============================================================================

Result<sc::json::Json> ConfigCenterClient::getConfig(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key.toStdString());
    if (it != m_cache.end()) {
        return Result<sc::json::Json>(*it);
    }

    return Result<sc::json::Json>(sc::json::Json());
}

Result<sc::json::Json> ConfigCenterClient::getConfig(const QString& key,
                                                      const sc::json::Json& defaultValue) {
    auto result = getConfig(key);
    if (result.isOk()) {
        return result;
    }
    return Result<sc::json::Json>(defaultValue);
}

// ============================================================================
// 配置写入
// ============================================================================

Result<void> ConfigCenterClient::setConfig(const QString& key, const sc::json::Json& value) {
    sc::json::Json oldVal;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key.toStdString());
        if (it != m_cache.end()) {
            oldVal = *it;
        }
        m_cache[key.toStdString()] = value;
    }

    notifyChange(key, oldVal, value);
    return Result<void>::ok();
}

Result<void> ConfigCenterClient::deleteConfig(const QString& key) {
    sc::json::Json oldVal;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key.toStdString());
        if (it != m_cache.end()) {
            oldVal = *it;
            m_cache.erase(it);
        }
    }

    notifyChange(key, oldVal, sc::json::Json());
    return Result<void>::ok();
}

// ============================================================================
// 批量操作
// ============================================================================

Result<sc::json::Json> ConfigCenterClient::getConfigs(const QString& prefix) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (prefix.isEmpty()) {
        return Result<sc::json::Json>(m_cache);
    }

    sc::json::Json result = sc::json::Json::object();
    std::string prefixStr = prefix.toStdString();
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.key().find(prefixStr) == 0) {
            result[it.key()] = it.value();
        }
    }
    return Result<sc::json::Json>(result);
}

// ============================================================================
// 配置监听
// ============================================================================

Result<void> ConfigCenterClient::watch(const QString& key, ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers[key].append(std::move(callback));
    return Result<void>::ok();
}

Result<void> ConfigCenterClient::unwatch(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers.remove(key);
    return Result<void>::ok();
}

// ============================================================================
// 配置版本
// ============================================================================

Result<qint64> ConfigCenterClient::getVersion(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_versions.find(key);
    if (it != m_versions.end()) {
        return Result<qint64>(it.value());
    }
    return Result<qint64>(0);
}

// ============================================================================
// 配置回滚
// ============================================================================

Result<void> ConfigCenterClient::rollback(const QString& key, qint64 version) {
    Q_UNUSED(key);
    Q_UNUSED(version);
    // 回滚需要后端支持，此处为 stub
    return Result<void>::ok();
}

// ============================================================================
// 后端信息
// ============================================================================

ConfigCenterBackend ConfigCenterClient::backend() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.backend;
}

// ============================================================================
// 属性优先级合并
// ============================================================================

sc::json::Json ConfigCenterClient::mergeConfig(const sc::json::Json& local,
                                                const sc::json::Json& remote) const {
    sc::json::Json merged = local;
    for (auto it = remote.begin(); it != remote.end(); ++it) {
        merged[it.key()] = it.value();
    }
    return merged;
}

// ============================================================================
// 内部监听定时器
// ============================================================================

void ConfigCenterClient::startWatchTimer() {
    if (!m_watchTimer) {
        m_watchTimer = new QTimer(this);
        QObject::connect(m_watchTimer, &QTimer::timeout, this, &ConfigCenterClient::onWatchTimerTick);
    }
    m_watchTimer->start(m_config.watchIntervalMs);
}

void ConfigCenterClient::stopWatchTimer() {
    if (m_watchTimer) {
        m_watchTimer->stop();
    }
}

void ConfigCenterClient::onWatchTimerTick() {
    // 轮询后端配置变更
    if (m_backend && m_connected) {
        auto result = m_backend->fetchConfig(m_config.namespace_);
        if (result.isOk()) {
            sc::json::Json remote = result.unwrap();
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto it = remote.begin(); it != remote.end(); ++it) {
                QString key = QString::fromStdString(it.key());
                auto cacheIt = m_cache.find(it.key());
                if (cacheIt == m_cache.end() || *cacheIt != it.value()) {
                    sc::json::Json oldVal = (cacheIt != m_cache.end()) ? *cacheIt : sc::json::Json();
                    m_cache[it.key()] = it.value();
                    notifyChange(key, oldVal, it.value());
                }
            }
        }
    }
}

// ============================================================================
// 变更通知
// ============================================================================

void ConfigCenterClient::notifyChange(const QString& key,
                                       const sc::json::Json& oldVal,
                                       const sc::json::Json& newVal) {
    ConfigChangeEvent event;
    event.key = key;
    event.oldValue = oldVal;
    event.newValue = newVal;
    event.timestamp = QDateTime::currentMSecsSinceEpoch();
    event.source = "local";

    emit configChanged(event);

    // 通知 watchers
    QList<ConfigChangeCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_watchers.find(key);
        if (it != m_watchers.end()) {
            callbacks = it.value();
        }
    }
    for (auto& cb : callbacks) {
        if (cb) {
            cb(event);
        }
    }
}

} // namespace sc