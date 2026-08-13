#ifndef SOUL_CORE_FEATURE_FLAGS_H
#define SOUL_CORE_FEATURE_FLAGS_H

// ============================================================================
// feature_flags.h — 灰度发布 / 功能开关 [v2.5.0]
// ============================================================================
// 对标 LaunchDarkly / Spring Cloud Feature Flags，提供:
//   - 布尔型开关 (Boolean Flag)
//   - 百分比灰度 (Percentage Rollout)
//   - 用户分组 (Targeting by User/Role)
//   - 时间窗口 (Scheduled Flag)
//   - 规则引擎 (Rule-based Flag)
//   - 与配置中心集成 (热更新)
//   - 本地缓存 + 远程同步
// ============================================================================

#include <QString>
#include <QHash>
#include <QSet>
#include <QDateTime>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "soul/core/result.h"

namespace sc {

// ============================================================================
// FeatureFlagType — 功能开关类型
// ============================================================================
enum class FeatureFlagType {
    Boolean,        // 布尔型: ON/OFF
    Percentage,     // 百分比灰度: 0-100%
    Targeted,       // 用户定向: 指定用户/角色/组
    Scheduled,      // 定时开关: 按时间窗口
    RuleBased,      // 规则引擎: 自定义规则
    KillSwitch      // 熔断开关: 紧急关闭
};

// ============================================================================
// FeatureFlagTarget — 目标条件
// ============================================================================
struct FeatureFlagTarget {
    QString userId;
    QString role;
    QString group;
    QString tenant;
    QHash<QString, QString> customAttributes;
};

// ============================================================================
// FeatureFlagRule — 自定义规则
// ============================================================================
struct FeatureFlagRule {
    QString attribute;     // 属性名
    QString op;            // 操作符: eq/ne/gt/lt/in/contains/startsWith/endsWith
    QString value;         // 期望值
    bool negate = false;   // 是否取反
};

// ============================================================================
// FeatureFlagConfig — 功能开关配置
// ============================================================================
struct FeatureFlagConfig {
    FeatureFlagType type = FeatureFlagType::Boolean;

    // Boolean 类型
    bool enabled = false;

    // Percentage 类型
    int percentage = 0;  // 0-100

    // Targeted 类型
    QSet<QString> allowedUsers;
    QSet<QString> allowedRoles;
    QSet<QString> allowedGroups;

    // Scheduled 类型
    QDateTime startTime;
    QDateTime endTime;

    // RuleBased 类型
    std::vector<FeatureFlagRule> rules;
    QString ruleLogic = "AND";  // AND / OR

    // 通用
    QString description;
    QString owner;
    qint64 version = 0;
    bool overrideLocal = false;  // 远程配置是否覆盖本地
};

// ============================================================================
// FeatureFlagSnapshot — 快照
// ============================================================================
struct FeatureFlagSnapshot {
    QString key;
    FeatureFlagConfig config;
    bool currentValue = false;
    qint64 lastUpdated = 0;
};

// ============================================================================
// IFeatureFlagProvider — 功能开关提供者接口
// ============================================================================
class IFeatureFlagProvider {
public:
    virtual ~IFeatureFlagProvider() = default;

    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;

    virtual Result<FeatureFlagConfig> getConfig(const QString& key) = 0;
    virtual Result<void> setConfig(const QString& key, const FeatureFlagConfig& config) = 0;
    virtual Result<void> deleteConfig(const QString& key) = 0;

    virtual Result<QHash<QString, FeatureFlagConfig>> getAllConfigs() = 0;

    using ConfigChangeCallback = std::function<void(const QString& key, const FeatureFlagConfig& config)>;
    virtual Result<void> watch(const QString& key, ConfigChangeCallback callback) = 0;
    virtual Result<void> unwatch(const QString& key) = 0;
};

// ============================================================================
// FeatureFlagManager — 功能开关管理器
// ============================================================================
class FeatureFlagManager {
public:
    static FeatureFlagManager& instance();

    // === 初始化 ===
    Result<void> initialize(std::unique_ptr<IFeatureFlagProvider> provider);
    void shutdown();

    // === 开关评估 ===
    bool isEnabled(const QString& key) const;
    bool isEnabled(const QString& key, const FeatureFlagTarget& target) const;
    bool isEnabled(const QString& key, bool defaultValue) const;

    // === 批量评估 ===
    QHash<QString, bool> evaluateAll() const;
    QHash<QString, bool> evaluateAll(const FeatureFlagTarget& target) const;

    // === 开关管理 ===
    Result<void> setFlag(const QString& key, const FeatureFlagConfig& config);
    Result<void> removeFlag(const QString& key);
    Result<FeatureFlagConfig> getFlagConfig(const QString& key) const;
    QHash<QString, FeatureFlagSnapshot> getAllSnapshots() const;

    // === 强制覆盖 ===
    void forceEnable(const QString& key);
    void forceDisable(const QString& key);
    void removeForce(const QString& key);
    bool isForced(const QString& key) const;

    // === 监听 ===
    using FlagChangeCallback = std::function<void(const QString& key, bool newValue)>;
    void onFlagChanged(const QString& key, FlagChangeCallback callback);
    void removeFlagListener(const QString& key);

    // === 快捷方法 ===
    bool isOn(const QString& key) const { return isEnabled(key); }
    bool isOff(const QString& key) const { return !isEnabled(key); }

private:
    FeatureFlagManager() = default;
    ~FeatureFlagManager() = default;

    bool evaluateBoolean(const FeatureFlagConfig& config) const;
    bool evaluatePercentage(const FeatureFlagConfig& config, const FeatureFlagTarget& target) const;
    bool evaluateTargeted(const FeatureFlagConfig& config, const FeatureFlagTarget& target) const;
    bool evaluateScheduled(const FeatureFlagConfig& config) const;
    bool evaluateRuleBased(const FeatureFlagConfig& config, const FeatureFlagTarget& target) const;
    bool evaluateRule(const FeatureFlagRule& rule, const FeatureFlagTarget& target) const;

    // 无锁内部评估 — 调用方必须已持有 m_mutex，供批量/快照复用，避免自递归加锁死锁
    bool evaluateEnabledUnlocked(const QString& key, const FeatureFlagTarget& target,
                                 bool defaultValue) const;

    int hashUser(const QString& userId) const;

    std::unique_ptr<IFeatureFlagProvider> m_provider;
    mutable std::mutex m_mutex;
    QHash<QString, bool> m_forceOverrides;
    QHash<QString, QList<FlagChangeCallback>> m_listeners;
    bool m_initialized = false;
};

} // namespace sc

#endif // SOUL_CORE_FEATURE_FLAGS_H