// ============================================================================
// json_feature_flag_provider.h — JSON/YAML 文件功能开关提供者 [v2.5.0 审计补全]
// ============================================================================
// 从 JSON/YAML 配置文件加载功能开关, 支持 QFileSystemWatcher 热更新。
// 配置格式:
// {
//   "flags": {
//     "new_checkout": { "type": "boolean", "enabled": true },
//     "dark_mode":    { "type": "percentage", "percentage": 30 },
//     "admin_only":   { "type": "targeted", "allowedRoles": ["admin"] }
//   }
// }
// ============================================================================

#ifndef SOUL_CORE_JSON_FEATURE_FLAG_PROVIDER_H
#define SOUL_CORE_JSON_FEATURE_FLAG_PROVIDER_H

#include "soul/core/feature_flags.h"
#include <QFileSystemWatcher>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

namespace sc {

class JsonFeatureFlagProvider : public IFeatureFlagProvider {
public:
    explicit JsonFeatureFlagProvider(const QString& configPath, bool enableWatch = true);
    ~JsonFeatureFlagProvider() override;

    Result<void> initialize() override;
    void shutdown() override;

    Result<FeatureFlagConfig> getConfig(const QString& key) override;
    Result<void> setConfig(const QString& key, const FeatureFlagConfig& config) override;
    Result<void> deleteConfig(const QString& key) override;

    Result<QHash<QString, FeatureFlagConfig>> getAllConfigs() override;

    Result<void> watch(const QString& key, ConfigChangeCallback callback) override;
    Result<void> unwatch(const QString& key) override;

private:
    Result<void> loadFromFile();
    FeatureFlagConfig parseFlagConfig(const QJsonObject& obj) const;
    void notifyWatchers();

    QString m_configPath;
    bool m_enableWatch;
    QFileSystemWatcher* m_watcher = nullptr;
    QHash<QString, FeatureFlagConfig> m_configs;
    QHash<QString, QList<ConfigChangeCallback>> m_watchers;
    mutable std::mutex m_mutex;
    bool m_initialized = false;
};

} // namespace sc

#endif // SOUL_CORE_JSON_FEATURE_FLAG_PROVIDER_H
