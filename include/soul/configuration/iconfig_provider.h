#ifndef SOUL_CONFIGURATION_ICONFIG_PROVIDER_H
#define SOUL_CONFIGURATION_ICONFIG_PROVIDER_H

#include <mutex>

// ============================================================================
// iconfig_provider.h — 统一配置 Provider 抽象 [v2.9.0 新增]
// ============================================================================
//
// 设计原则:
//   - Provider 负责从特定来源加载配置 (文件/环境变量/命令行/远程)
//   - 每个 Provider 产生一个不可变的 ConfigSnapshot
//   - 优先级链: CommandLine > Environment > Remote > LocalFile > Default
//   - 高层级 Provider 的值覆盖低层级
//
// 与现有系统的关系:
//   - 现有 IConfiguration (JsonConfiguration/IniConfiguration) 继续作为底层实现
//   - 现有 Config 单例继续作为统一入口
//   - 新增 IConfigProvider 抽象层 — 封装加载逻辑，返回 Snapshot
//   - RemoteConfigProvider 为 Nacos/Etcd 等 Adapter 提供统一边界
//
// 用法:
//   auto chain = std::make_shared<PriorityConfigChain>();
//   chain->addProvider(std::make_shared<CommandLineConfigProvider>(argc, argv));
//   chain->addProvider(std::make_shared<EnvironmentConfigProvider>("SOUL_"));
//   chain->addProvider(std::make_shared<JsonFileConfigProvider>("config/app.json"));
//   chain->addProvider(std::make_shared<DefaultConfigProvider>(defaults));
//
//   auto result = chain->load();
//   if (result.isOk()) {
//       auto snapshot = result.unwrap();
//       auto host = snapshot.getString("server.host").value_or("0.0.0.0");
//       auto port = snapshot.getInt("server.port").value_or(8080);
//   }

#include <QString>
#include <QVariant>
#include <QHash>
#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include "soul/core/result.h"
#include "soul/core/error.h"

namespace sc {

// ============================================================================
// ConfigSnapshot — 不可变配置快照 [v2.9.0]
// ============================================================================
//
// 线程安全: 构造后不可变，多线程读安全
// 性能: O(log n) 查找，零拷贝共享

class ConfigSnapshot {
public:
    ConfigSnapshot() = default;

    explicit ConfigSnapshot(QHash<QString, QVariant> values)
        : m_values(std::move(values)) {}

    // === 读取 ===

    std::optional<QString> getString(const QString& key) const {
        auto it = m_values.find(key);
        if (it != m_values.end() && it->isValid()) {
            return it->toString();
        }
        return std::nullopt;
    }

    std::optional<int> getInt(const QString& key) const {
        auto it = m_values.find(key);
        if (it != m_values.end() && it->isValid()) {
            bool ok = false;
            int val = it->toInt(&ok);
            if (ok) return val;
        }
        return std::nullopt;
    }

    std::optional<int64_t> getInt64(const QString& key) const {
        auto it = m_values.find(key);
        if (it != m_values.end() && it->isValid()) {
            bool ok = false;
            int64_t val = it->toLongLong(&ok);
            if (ok) return val;
        }
        return std::nullopt;
    }

    std::optional<double> getDouble(const QString& key) const {
        auto it = m_values.find(key);
        if (it != m_values.end() && it->isValid()) {
            bool ok = false;
            double val = it->toDouble(&ok);
            if (ok) return val;
        }
        return std::nullopt;
    }

    std::optional<bool> getBool(const QString& key) const {
        auto it = m_values.find(key);
        if (it != m_values.end() && it->isValid()) {
            return it->toBool();
        }
        return std::nullopt;
    }

    QVariant getValue(const QString& key) const {
        return m_values.value(key);
    }

    // === 便捷 (带默认值) ===

    QString getStringOr(const QString& key, const QString& defaultValue) const {
        return getString(key).value_or(defaultValue);
    }

    int getIntOr(const QString& key, int defaultValue) const {
        return getInt(key).value_or(defaultValue);
    }

    int64_t getInt64Or(const QString& key, int64_t defaultValue) const {
        return getInt64(key).value_or(defaultValue);
    }

    double getDoubleOr(const QString& key, double defaultValue) const {
        return getDouble(key).value_or(defaultValue);
    }

    bool getBoolOr(const QString& key, bool defaultValue) const {
        return getBool(key).value_or(defaultValue);
    }

    // === 查询 ===

    bool contains(const QString& key) const { return m_values.contains(key); }
    size_t size() const { return m_values.size(); }
    bool isEmpty() const { return m_values.isEmpty(); }

    const QHash<QString, QVariant>& all() const { return m_values; }

    /// @brief 合并另一个 Snapshot (高优先级的值覆盖低优先级)
    ConfigSnapshot merge(const ConfigSnapshot& higher) const {
        QHash<QString, QVariant> merged = m_values;
        for (auto it = higher.m_values.begin(); it != higher.m_values.end(); ++it) {
            merged.insert(it.key(), it.value());
        }
        return ConfigSnapshot(std::move(merged));
    }

    // === 元数据 ===

    void setVersion(const QString& v) { m_version = v; }
    QString version() const { return m_version; }

    void setLoadedAt(std::chrono::system_clock::time_point t) { m_loadedAt = t; }
    std::chrono::system_clock::time_point loadedAt() const { return m_loadedAt; }

    void setSourceName(const QString& name) { m_sourceName = name; }
    QString sourceName() const { return m_sourceName; }

private:
    QHash<QString, QVariant> m_values;
    QString m_version;
    QString m_sourceName;
    std::chrono::system_clock::time_point m_loadedAt;
};

// ============================================================================
// IConfigProvider — 配置 Provider 接口 [v2.9.0]
// ============================================================================
//
// 每个 Provider 负责从特定来源加载配置并返回不可变 Snapshot。
// Provider 不持有可变状态 — 每次 load() 产生新的 Snapshot。

class IConfigProvider {
public:
    virtual ~IConfigProvider() = default;

    /// @brief 加载配置
    /// @return ConfigSnapshot 或 Error
    /// @note 每次调用产生新的 Snapshot，线程安全
    virtual Result<ConfigSnapshot> load() = 0;

    /// @brief Provider 名称 (用于日志/诊断)
    virtual std::string name() const = 0;

    /// @brief 优先级 (越大越优先，用于优先级链排序)
    virtual int priority() const = 0;
};

// ============================================================================
// PriorityConfigChain — 优先级配置链 [v2.9.0]
// ============================================================================
//
// 按 priority 降序排列 Provider，高优先级的值覆盖低优先级。
// 优先级: CommandLine (300) > Environment (200) > Remote (100) > LocalFile (50) > Default (0)
//
// 加载失败处理:
//   - Default/LocalFile Provider 失败 → 返回 Error (关键配置)
//   - Remote Provider 失败 → 跳过 (非关键，允许降级)
//   - Environment/CommandLine Provider 失败 → 返回 Error

class PriorityConfigChain : public IConfigProvider {
public:
    /// @brief 添加 Provider (自动按优先级排序)
    void addProvider(std::shared_ptr<IConfigProvider> provider);

    /// @brief 加载所有 Provider 并按优先级合并
    Result<ConfigSnapshot> load() override;

    std::string name() const override { return "PriorityConfigChain"; }
    int priority() const override { return 0; }  // Chain 自身无优先级

    /// @brief 尝试重新加载 (仅重新加载 Remote Provider)
    /// @return 新的 Snapshot，失败时保持旧配置不变
    Result<ConfigSnapshot> tryReload();

    /// @brief 获取当前活跃的 Snapshot
    ConfigSnapshot currentSnapshot() const;

private:
    std::vector<std::shared_ptr<IConfigProvider>> m_providers;
    ConfigSnapshot m_currentSnapshot;
    mutable std::mutex m_mutex;
};

// ============================================================================
// 预定义优先级常量
// ============================================================================

namespace ConfigPriority {
    constexpr int Default     = 0;    // 默认值
    constexpr int LocalFile   = 50;   // 本地配置文件
    constexpr int Remote      = 100;  // 远程配置中心
    constexpr int Environment = 200;  // 环境变量
    constexpr int CommandLine = 300;  // 命令行参数
}

} // namespace sc

#endif // SOUL_CONFIGURATION_ICONFIG_PROVIDER_H
