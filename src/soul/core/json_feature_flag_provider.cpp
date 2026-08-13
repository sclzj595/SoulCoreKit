#include "soul/core/json_feature_flag_provider.h"
#include <QDebug>

namespace sc {

JsonFeatureFlagProvider::JsonFeatureFlagProvider(const QString& configPath, bool enableWatch)
    : m_configPath(configPath), m_enableWatch(enableWatch) {}

JsonFeatureFlagProvider::~JsonFeatureFlagProvider() {
    shutdown();
}

Result<void> JsonFeatureFlagProvider::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto result = loadFromFile();
    if (result.isErr()) {
        return result;
    }
    if (m_enableWatch) {
        m_watcher = new QFileSystemWatcher();
        m_watcher->addPath(m_configPath);
        QObject::connect(m_watcher, &QFileSystemWatcher::fileChanged, [this](const QString&) {
            std::lock_guard<std::mutex> guard(m_mutex);  // 避免 -Wshadow
            loadFromFile();
            notifyWatchers();
        });
    }
    m_initialized = true;
    return Result<void>::ok();
}

void JsonFeatureFlagProvider::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = false;
    if (m_watcher) {
        delete m_watcher;
        m_watcher = nullptr;
    }
    m_watchers.clear();
    m_configs.clear();
}

Result<FeatureFlagConfig> JsonFeatureFlagProvider::getConfig(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) {
        return Result<FeatureFlagConfig>::err(Error(ErrorCode::InternalError,
            "JsonFeatureFlagProvider not initialized"));
    }
    auto it = m_configs.find(key);
    if (it != m_configs.end()) {
        return Result<FeatureFlagConfig>::ok(it.value());
    }
    return Result<FeatureFlagConfig>::err(Error(ErrorCode::NotFound,
        QString("Flag not found: %1").arg(key)));
}

Result<void> JsonFeatureFlagProvider::setConfig(const QString& key, const FeatureFlagConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) {
        return Result<void>::err(Error(ErrorCode::InternalError,
            "JsonFeatureFlagProvider not initialized"));
    }
    m_configs[key] = config;
    return Result<void>::ok();
}

Result<void> JsonFeatureFlagProvider::deleteConfig(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) {
        return Result<void>::err(Error(ErrorCode::InternalError,
            "JsonFeatureFlagProvider not initialized"));
    }
    m_configs.remove(key);
    return Result<void>::ok();
}

Result<QHash<QString, FeatureFlagConfig>> JsonFeatureFlagProvider::getAllConfigs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) {
        return Result<QHash<QString, FeatureFlagConfig>>::err(Error(ErrorCode::InternalError,
            "JsonFeatureFlagProvider not initialized"));
    }
    return Result<QHash<QString, FeatureFlagConfig>>::ok(m_configs);
}

Result<void> JsonFeatureFlagProvider::watch(const QString& key, ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers[key].append(std::move(callback));
    return Result<void>::ok();
}

Result<void> JsonFeatureFlagProvider::unwatch(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers.remove(key);
    return Result<void>::ok();
}

// ============================================================================
// 内部实现
// ============================================================================

Result<void> JsonFeatureFlagProvider::loadFromFile() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<void>::err(Error(ErrorCode::NotFound,
            QString("Cannot open feature flag config: %1").arg(m_configPath)));
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        return Result<void>::err(Error(ErrorCode::InternalError,
            QString("JSON parse error at offset %1: %2")
                .arg(parseError.offset)
                .arg(parseError.errorString())));
    }

    QJsonObject root = doc.object();
    QJsonObject flags = root.value("flags").toObject();
    m_configs.clear();

    for (auto it = flags.begin(); it != flags.end(); ++it) {
        QJsonObject flagObj = it.value().toObject();
        m_configs[it.key()] = parseFlagConfig(flagObj);
    }

    qDebug() << "JsonFeatureFlagProvider: loaded" << m_configs.size()
             << "flags from" << m_configPath;
    return Result<void>::ok();
}

FeatureFlagConfig JsonFeatureFlagProvider::parseFlagConfig(const QJsonObject& obj) const {
    FeatureFlagConfig cfg;

    QString typeStr = obj.value("type").toString("boolean");
    if (typeStr == "boolean")       cfg.type = FeatureFlagType::Boolean;
    else if (typeStr == "percentage") cfg.type = FeatureFlagType::Percentage;
    else if (typeStr == "targeted")   cfg.type = FeatureFlagType::Targeted;
    else if (typeStr == "scheduled")  cfg.type = FeatureFlagType::Scheduled;
    else if (typeStr == "rule_based") cfg.type = FeatureFlagType::RuleBased;
    else if (typeStr == "killswitch") cfg.type = FeatureFlagType::KillSwitch;

    cfg.enabled       = obj.value("enabled").toBool(false);
    cfg.percentage    = obj.value("percentage").toInt(0);
    cfg.description   = obj.value("description").toString();
    cfg.owner         = obj.value("owner").toString();
    cfg.version       = static_cast<qint64>(obj.value("version").toDouble(0));
    cfg.overrideLocal = obj.value("overrideLocal").toBool(false);

    // targeted: allowedUsers / allowedRoles / allowedGroups
    auto toSet = [](const QJsonArray& arr) -> QSet<QString> {
        QSet<QString> s;
        for (const auto& v : arr) s.insert(v.toString());
        return s;
    };
    cfg.allowedUsers  = toSet(obj.value("allowedUsers").toArray());
    cfg.allowedRoles  = toSet(obj.value("allowedRoles").toArray());
    cfg.allowedGroups = toSet(obj.value("allowedGroups").toArray());

    // scheduled
    if (obj.contains("startTime")) {
        cfg.startTime = QDateTime::fromString(obj.value("startTime").toString(), Qt::ISODate);
    }
    if (obj.contains("endTime")) {
        cfg.endTime = QDateTime::fromString(obj.value("endTime").toString(), Qt::ISODate);
    }

    // rules
    if (obj.contains("rules")) {
        QJsonArray rulesArr = obj.value("rules").toArray();
        for (const auto& r : rulesArr) {
            QJsonObject ruleObj = r.toObject();
            FeatureFlagRule rule;
            rule.attribute = ruleObj.value("attribute").toString();
            rule.op        = ruleObj.value("op").toString();
            rule.value     = ruleObj.value("value").toString();
            rule.negate    = ruleObj.value("negate").toBool(false);
            cfg.rules.push_back(rule);
        }
    }
    cfg.ruleLogic = obj.value("ruleLogic").toString("AND");

    return cfg;
}

void JsonFeatureFlagProvider::notifyWatchers() {
    for (auto it = m_configs.begin(); it != m_configs.end(); ++it) {
        auto wit = m_watchers.find(it.key());
        if (wit != m_watchers.end()) {
            for (const auto& cb : wit.value()) {
                if (cb) cb(it.key(), it.value());
            }
        }
    }
}

} // namespace sc
