#ifndef SOUL_CONFIGURATION_CONFIG_PROVIDERS_H
#define SOUL_CONFIGURATION_CONFIG_PROVIDERS_H

// ============================================================================
// config_providers.h — 内置 ConfigProvider 实现 [v2.9.0 新增]
// ============================================================================
//
// 提供 File / Environment / CommandLine / Default 四种内置 Provider。
// Remote Provider 接口见 iconfig_provider.h + remote_config.h。

#include "soul/configuration/iconfig_provider.h"
#include "soul/configuration/iconfiguration.h"
#include "soul/configuration/remote_config.h"
#include <QString>
#include <QHash>
#include <QVariant>
#include <memory>

namespace sc {

// ============================================================================
// JsonFileConfigProvider — JSON 文件 Provider
// ============================================================================
//
// 加载 JSON 配置文件，使用现有的 JsonConfiguration。
// 文件名不存在时返回 Error (MissingConfig)。

class JsonFileConfigProvider : public IConfigProvider {
public:
    explicit JsonFileConfigProvider(const QString& filePath,
                                     int prio = ConfigPriority::LocalFile);

    Result<ConfigSnapshot> load() override;
    std::string name() const override { return "JsonFile:" + m_filePath.toStdString(); }
    int priority() const override { return m_priority; }

private:
    QString m_filePath;
    int m_priority;
};

// ============================================================================
// IniFileConfigProvider — INI 文件 Provider
// ============================================================================

class IniFileConfigProvider : public IConfigProvider {
public:
    explicit IniFileConfigProvider(const QString& filePath,
                                    int prio = ConfigPriority::LocalFile);

    Result<ConfigSnapshot> load() override;
    std::string name() const override { return "IniFile:" + m_filePath.toStdString(); }
    int priority() const override { return m_priority; }

private:
    QString m_filePath;
    int m_priority;
};

// ============================================================================
// EnvironmentConfigProvider — 环境变量 Provider
// ============================================================================
//
// 读取进程环境变量。key 前缀 (如 "SOUL_") 会被自动去除。
// 例如: SOUL_SERVER_PORT → server.port

class EnvironmentConfigProvider : public IConfigProvider {
public:
    /// @param prefix 环境变量前缀 (如 "SOUL_", "APP_")
    /// @param prio 优先级
    explicit EnvironmentConfigProvider(const QString& prefix = "SOUL_",
                                        int prio = ConfigPriority::Environment);

    Result<ConfigSnapshot> load() override;
    std::string name() const override { return "Environment"; }
    int priority() const override { return m_priority; }

private:
    QString m_prefix;
    int m_priority;

    static QString envKeyToConfigKey(const QString& prefix, const QString& envKey);
};

// ============================================================================
// CommandLineConfigProvider — 命令行参数 Provider
// ============================================================================
//
// 解析 --key=value 格式的命令行参数。
// 支持: --server.port=8080, --db.host=localhost

class CommandLineConfigProvider : public IConfigProvider {
public:
    /// @param argc, argv 命令行参数
    /// @param prio 优先级
    CommandLineConfigProvider(int argc, char* argv[],
                               int prio = ConfigPriority::CommandLine);

    Result<ConfigSnapshot> load() override;
    std::string name() const override { return "CommandLine"; }
    int priority() const override { return m_priority; }

private:
    std::vector<std::string> m_args;
    int m_priority;

    static std::pair<QString, QString> parseArg(const std::string& arg);
};

// ============================================================================
// DefaultConfigProvider — 默认值 Provider
// ============================================================================
//
// 提供硬编码的默认配置值，优先级最低。

class DefaultConfigProvider : public IConfigProvider {
public:
    DefaultConfigProvider();

    /// @brief 设置默认值
    void setDefault(const QString& key, const QVariant& value);

    Result<ConfigSnapshot> load() override;
    std::string name() const override { return "Default"; }
    int priority() const override { return ConfigPriority::Default; }

private:
    QHash<QString, QVariant> m_defaults;
};

// ============================================================================
// RemoteConfigProvider — 远程配置 Provider (Adapter 边界) [v2.9.0]
// ============================================================================
//
// 封装 IRemoteConfigSource，提供统一的 Provider 接口。
// 加载失败时返回 Error，由 PriorityConfigChain 决定是否降级。

class RemoteConfigProvider : public IConfigProvider {
public:
    /// @param source 远程配置源 (Nacos/Etcd/...)
    /// @param namespaceName 命名空间
    /// @param prio 优先级
    RemoteConfigProvider(std::shared_ptr<IRemoteConfigSource> source,
                          const QString& namespaceName = "application",
                          int prio = ConfigPriority::Remote);

    Result<ConfigSnapshot> load() override;
    std::string name() const override { return "Remote:" + m_namespace.toStdString(); }
    int priority() const override { return m_priority; }

private:
    std::shared_ptr<IRemoteConfigSource> m_source;
    QString m_namespace;
    int m_priority;
};

} // namespace sc

#endif // SOUL_CONFIGURATION_CONFIG_PROVIDERS_H
