// ============================================================================
// feature_flags.cpp — 灰度发布 / 功能开关实现 [v2.5.0]
// ============================================================================

#include "soul/core/feature_flags.h"
#include "soul/core/error.h"

#include <QCryptographicHash>
#include <QDebug>

namespace sc {

// ============================================================================
// InMemoryFeatureFlagProvider — IFeatureFlagProvider 内存实现
// ============================================================================

class InMemoryFeatureFlagProvider : public IFeatureFlagProvider {
public:
    Result<void> initialize() override {
        return Result<void>::ok();
    }

    void shutdown() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_configs.clear();
        m_watchers.clear();
    }

    Result<FeatureFlagConfig> getConfig(const QString& key) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_configs.find(key);
        if (it != m_configs.end()) {
            return Result<FeatureFlagConfig>(it.value());
        }
        return Result<FeatureFlagConfig>::err(Error(ErrorCode::NotFound,
            QString("Feature flag not found: %1").arg(key)));
    }

    Result<void> setConfig(const QString& key, const FeatureFlagConfig& config) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_configs.insert(key, config);

        // 通知 watchers
        auto it = m_watchers.find(key);
        if (it != m_watchers.end()) {
            for (auto& cb : it.value()) {
                if (cb) {
                    cb(key, config);
                }
            }
        }

        return Result<void>::ok();
    }

    Result<void> deleteConfig(const QString& key) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_configs.remove(key);
        return Result<void>::ok();
    }

    Result<QHash<QString, FeatureFlagConfig>> getAllConfigs() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return Result<QHash<QString, FeatureFlagConfig>>(m_configs);
    }

    Result<void> watch(const QString& key, ConfigChangeCallback callback) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_watchers[key].append(std::move(callback));
        return Result<void>::ok();
    }

    Result<void> unwatch(const QString& key) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_watchers.remove(key);
        return Result<void>::ok();
    }

private:
    std::mutex m_mutex;
    QHash<QString, FeatureFlagConfig> m_configs;
    QHash<QString, QList<ConfigChangeCallback>> m_watchers;
};

// ============================================================================
// FeatureFlagManager 实现
// ============================================================================

FeatureFlagManager& FeatureFlagManager::instance() {
    static FeatureFlagManager s_instance;
    return s_instance;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> FeatureFlagManager::initialize(std::unique_ptr<IFeatureFlagProvider> provider) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        return Result<void>::err(Error(ErrorCode::AlreadyExists,
            "FeatureFlagManager already initialized"));
    }

    if (!provider) {
        provider = std::make_unique<InMemoryFeatureFlagProvider>();
    }

    auto result = provider->initialize();
    if (result.isErr()) {
        return result;
    }

    m_provider = std::move(provider);
    m_initialized = true;

    return Result<void>::ok();
}

void FeatureFlagManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_provider) {
        m_provider->shutdown();
    }
    m_provider.reset();
    m_forceOverrides.clear();
    m_listeners.clear();
    m_initialized = false;
}

// ============================================================================
// 开关评估 — isEnabled
// ============================================================================

bool FeatureFlagManager::isEnabled(const QString& key) const {
    return isEnabled(key, false);
}

bool FeatureFlagManager::isEnabled(const QString& key, const FeatureFlagTarget& target) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 强制覆盖优先
    auto forceIt = m_forceOverrides.find(key);
    if (forceIt != m_forceOverrides.end()) {
        return forceIt.value();
    }

    if (!m_provider) return false;

    auto result = m_provider->getConfig(key);
    if (result.isErr()) return false;

    const auto& config = result.unwrap();

    switch (config.type) {
    case FeatureFlagType::Boolean:
        return evaluateBoolean(config);
    case FeatureFlagType::Percentage:
        return evaluatePercentage(config, target);
    case FeatureFlagType::Targeted:
        return evaluateTargeted(config, target);
    case FeatureFlagType::Scheduled:
        return evaluateScheduled(config);
    case FeatureFlagType::RuleBased:
        return evaluateRuleBased(config, target);
    case FeatureFlagType::KillSwitch:
        return !config.enabled;  // KillSwitch: enabled=true 表示关闭功能
    }

    return config.enabled;
}

bool FeatureFlagManager::isEnabled(const QString& key, bool defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto forceIt = m_forceOverrides.find(key);
    if (forceIt != m_forceOverrides.end()) {
        return forceIt.value();
    }

    if (!m_provider) return defaultValue;

    auto result = m_provider->getConfig(key);
    if (result.isErr()) return defaultValue;

    return isEnabled(key, FeatureFlagTarget{});
}

// ============================================================================
// 批量评估
// ============================================================================

QHash<QString, bool> FeatureFlagManager::evaluateAll() const {
    return evaluateAll(FeatureFlagTarget{});
}

QHash<QString, bool> FeatureFlagManager::evaluateAll(const FeatureFlagTarget& target) const {
    QHash<QString, bool> results;
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_provider) return results;

    auto configsResult = m_provider->getAllConfigs();
    if (configsResult.isErr()) return results;

    const auto& configs = configsResult.unwrap();
    for (auto it = configs.constBegin(); it != configs.constEnd(); ++it) {
        results.insert(it.key(), isEnabled(it.key(), target));
    }

    return results;
}

// ============================================================================
// 开关管理
// ============================================================================

Result<void> FeatureFlagManager::setFlag(const QString& key, const FeatureFlagConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_provider) {
        return Result<void>::err(Error(ErrorCode::NotConnected, "No provider set"));
    }

    return m_provider->setConfig(key, config);
}

Result<void> FeatureFlagManager::removeFlag(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_forceOverrides.remove(key);

    if (!m_provider) {
        return Result<void>::err(Error(ErrorCode::NotConnected, "No provider set"));
    }

    return m_provider->deleteConfig(key);
}

Result<FeatureFlagConfig> FeatureFlagManager::getFlagConfig(const QString& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_provider) {
        return Result<FeatureFlagConfig>::err(Error(ErrorCode::NotConnected, "No provider set"));
    }

    return m_provider->getConfig(key);
}

QHash<QString, FeatureFlagSnapshot> FeatureFlagManager::getAllSnapshots() const {
    QHash<QString, FeatureFlagSnapshot> snapshots;
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_provider) return snapshots;

    auto configsResult = m_provider->getAllConfigs();
    if (configsResult.isErr()) return snapshots;

    const auto& configs = configsResult.unwrap();
    for (auto it = configs.constBegin(); it != configs.constEnd(); ++it) {
        FeatureFlagSnapshot snapshot;
        snapshot.key = it.key();
        snapshot.config = it.value();
        snapshot.currentValue = isEnabled(it.key());
        snapshot.lastUpdated = QDateTime::currentMSecsSinceEpoch();
        snapshots.insert(it.key(), snapshot);
    }

    return snapshots;
}

// ============================================================================
// 强制覆盖
// ============================================================================

void FeatureFlagManager::forceEnable(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_forceOverrides.insert(key, true);
}

void FeatureFlagManager::forceDisable(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_forceOverrides.insert(key, false);
}

void FeatureFlagManager::removeForce(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_forceOverrides.remove(key);
}

bool FeatureFlagManager::isForced(const QString& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_forceOverrides.contains(key);
}

// ============================================================================
// 监听
// ============================================================================

void FeatureFlagManager::onFlagChanged(const QString& key, FlagChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners[key].append(std::move(callback));
}

void FeatureFlagManager::removeFlagListener(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.remove(key);
}

// ============================================================================
// 评估逻辑实现
// ============================================================================

bool FeatureFlagManager::evaluateBoolean(const FeatureFlagConfig& config) const {
    return config.enabled;
}

bool FeatureFlagManager::evaluatePercentage(const FeatureFlagConfig& config,
                                             const FeatureFlagTarget& target) const {
    if (config.percentage <= 0) return false;
    if (config.percentage >= 100) return true;

    int hash = hashUser(target.userId);
    int bucket = hash % 100;
    return bucket < config.percentage;
}

bool FeatureFlagManager::evaluateTargeted(const FeatureFlagConfig& config,
                                           const FeatureFlagTarget& target) const {
    if (!target.userId.isEmpty() && config.allowedUsers.contains(target.userId)) {
        return true;
    }
    if (!target.role.isEmpty() && config.allowedRoles.contains(target.role)) {
        return true;
    }
    if (!target.group.isEmpty() && config.allowedGroups.contains(target.group)) {
        return true;
    }
    return false;
}

bool FeatureFlagManager::evaluateScheduled(const FeatureFlagConfig& config) const {
    QDateTime now = QDateTime::currentDateTime();

    if (config.startTime.isValid() && now < config.startTime) {
        return false;
    }
    if (config.endTime.isValid() && now > config.endTime) {
        return false;
    }

    return config.enabled;
}

bool FeatureFlagManager::evaluateRuleBased(const FeatureFlagConfig& config,
                                            const FeatureFlagTarget& target) const {
    if (config.rules.empty()) return config.enabled;

    bool result = (config.ruleLogic == "AND");

    for (const auto& rule : config.rules) {
        bool match = evaluateRule(rule, target);

        if (config.ruleLogic == "AND") {
            if (!match) return false;
        } else {  // OR
            if (match) return true;
        }
    }

    return result;
}

bool FeatureFlagManager::evaluateRule(const FeatureFlagRule& rule,
                                       const FeatureFlagTarget& target) const {
    QString attrValue;

    if (rule.attribute == "userId") {
        attrValue = target.userId;
    } else if (rule.attribute == "role") {
        attrValue = target.role;
    } else if (rule.attribute == "group") {
        attrValue = target.group;
    } else if (rule.attribute == "tenant") {
        attrValue = target.tenant;
    } else {
        attrValue = target.customAttributes.value(rule.attribute);
    }

    bool match = false;
    if (rule.op == "eq") {
        match = (attrValue == rule.value);
    } else if (rule.op == "ne") {
        match = (attrValue != rule.value);
    } else if (rule.op == "contains") {
        match = attrValue.contains(rule.value);
    } else if (rule.op == "startsWith") {
        match = attrValue.startsWith(rule.value);
    } else if (rule.op == "endsWith") {
        match = attrValue.endsWith(rule.value);
    } else {
        match = false;
    }

    return rule.negate ? !match : match;
}

int FeatureFlagManager::hashUser(const QString& userId) const {
    if (userId.isEmpty()) {
        return QDateTime::currentMSecsSinceEpoch() % 100;
    }

    QByteArray hash = QCryptographicHash::hash(
        userId.toUtf8(), QCryptographicHash::Md5);
    quint32 val = 0;
    for (int i = 0; i < qMin(hash.size(), 4); ++i) {
        val = (val << 8) | static_cast<quint32>(static_cast<unsigned char>(hash[i]));
    }
    return static_cast<int>(val);
}

} // namespace sc