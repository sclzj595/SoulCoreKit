#include "soul/configuration/config.h"
#include "soul/configuration/json_configuration.h"
#include "soul/logging/log_macros.h"
#include "soul/core/error.h"
#include <memory>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
#include <QProcessEnvironment>

namespace sc {

Config::Config() {
    QObject::connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged,
                     this, &Config::onFileChanged);
}

void Config::init() {
}

void Config::shutdown() {
    (void)saveAll();
}

Config::~Config() {
}

Result<void> Config::loadFromDirectory(const QString& configDir) {
    QDir dir(configDir);
    if (!dir.exists()) {
        SC_WARN("Config directory does not exist: " + configDir.toStdString());
        return Error(ErrorCode::NotFound, "Config directory does not exist: " + configDir.toStdString());
    }

    QStringList filters;
    filters << "*.json";
    QStringList files = dir.entryList(filters, QDir::Files);

    bool hotReload;
    {
        QMutexLocker lock(&m_dataMutex);
        m_configDir = configDir;
        for (const QString& file : files) {
            loadJsonFileLocked(dir.filePath(file));
        }
        hotReload = m_hotReloadEnabled;
    }

    if (hotReload) {
        for (const QString& file : files) {
            m_fileWatcher.addPath(dir.filePath(file));
        }
    }

    SC_INFO("Loaded config from directory: " + configDir.toStdString());
    return {};
}

Result<void> Config::loadFile(const QString& filePath) {
    QMutexLocker lock(&m_dataMutex);
    if (loadJsonFileLocked(filePath)) {
        return {};
    }
    return Error(ErrorCode::InternalError, "Failed to load config file: " + filePath.toStdString());
}

Result<void> Config::loadEnvironment(const QString& env) {
    QString envDir;
    {
        QMutexLocker lock(&m_dataMutex);
        m_currentEnv = env;
        envDir = QDir(m_configDir).filePath(env);
    }

    QDir dir(envDir);
    if (!dir.exists()) {
        SC_WARN("Environment directory does not exist: " + envDir.toStdString());
        return Error(ErrorCode::NotFound, "Environment directory does not exist: " + envDir.toStdString());
    }

    QStringList filters;
    filters << "*.json";
    QStringList files = dir.entryList(filters, QDir::Files);

    bool hotReload;
    {
        QMutexLocker lock(&m_dataMutex);
        for (const QString& file : files) {
            loadJsonFileLocked(dir.filePath(file));
        }
        hotReload = m_hotReloadEnabled;
    }

    if (hotReload) {
        for (const QString& file : files) {
            m_fileWatcher.addPath(dir.filePath(file));
        }
    }

    SC_INFO("Loaded environment config: " + env.toStdString());
    return {};
}

QString Config::getString(const QString& key, const QString& defaultValue) const {
    QMutexLocker lock(&m_dataMutex);
    QVariant value = getValueLocked(key);
    if (value.isValid()) {
        return value.toString();
    }
    return defaultValue;
}

int Config::getInt(const QString& key, int defaultValue) const {
    QMutexLocker lock(&m_dataMutex);
    QVariant value = getValueLocked(key);
    if (value.isValid()) {
        return value.toInt();
    }
    return defaultValue;
}

double Config::getDouble(const QString& key, double defaultValue) const {
    QMutexLocker lock(&m_dataMutex);
    QVariant value = getValueLocked(key);
    if (value.isValid()) {
        return value.toDouble();
    }
    return defaultValue;
}

bool Config::getBool(const QString& key, bool defaultValue) const {
    QMutexLocker lock(&m_dataMutex);
    QVariant value = getValueLocked(key);
    if (!value.isValid()) return defaultValue;
    if (value.typeId() == QMetaType::Bool) return value.toBool();
    if (value.typeId() == QMetaType::QString) {
        QString str = value.toString().trimmed().toLower();
        if (str == "true" || str == "1" || str == "yes") return true;
        if (str == "false" || str == "0" || str == "no") return false;
    }
    return value.toBool();
}

void Config::setString(const QString& key, const QString& value) {
    std::vector<ConfigChangeCallback> callbacksCopy;
    {
        QMutexLocker lock(&m_dataMutex);
        setValueLocked(key, value);
        callbacksCopy.reserve(m_changeCallbacks.size());
        for (const auto& pair : m_changeCallbacks) {
            callbacksCopy.push_back(pair.second);
        }
    }
    for (const auto& callback : callbacksCopy) {
        callback(key);
    }
}

void Config::setInt(const QString& key, int value) {
    std::vector<ConfigChangeCallback> callbacksCopy;
    {
        QMutexLocker lock(&m_dataMutex);
        setValueLocked(key, value);
        callbacksCopy.reserve(m_changeCallbacks.size());
        for (const auto& pair : m_changeCallbacks) {
            callbacksCopy.push_back(pair.second);
        }
    }
    for (const auto& callback : callbacksCopy) {
        callback(key);
    }
}

void Config::setDouble(const QString& key, double value) {
    std::vector<ConfigChangeCallback> callbacksCopy;
    {
        QMutexLocker lock(&m_dataMutex);
        setValueLocked(key, value);
        callbacksCopy.reserve(m_changeCallbacks.size());
        for (const auto& pair : m_changeCallbacks) {
            callbacksCopy.push_back(pair.second);
        }
    }
    for (const auto& callback : callbacksCopy) {
        callback(key);
    }
}

void Config::setBool(const QString& key, bool value) {
    std::vector<ConfigChangeCallback> callbacksCopy;
    {
        QMutexLocker lock(&m_dataMutex);
        setValueLocked(key, value);
        callbacksCopy.reserve(m_changeCallbacks.size());
        for (const auto& pair : m_changeCallbacks) {
            callbacksCopy.push_back(pair.second);
        }
    }
    for (const auto& callback : callbacksCopy) {
        callback(key);
    }
}

bool Config::contains(const QString& key) const {
    QMutexLocker lock(&m_dataMutex);
    return getValueLocked(key).isValid();
}

void Config::remove(const QString& key) {
    QMutexLocker lock(&m_dataMutex);
    for (auto& source : m_configSources) {
        source->remove(key);
    }
}

Result<void> Config::saveToFile(const QString& filePath) const {
    QMutexLocker lock(&m_dataMutex);
    if (m_configSources.empty()) {
        return Error(ErrorCode::InternalError, "No config sources to save");
    }

    // 合并所有配置源的数据到单个 QJsonObject
    QJsonObject merged;
    for (const auto& source : m_configSources) {
        const auto& jsonData = dynamic_cast<const JsonConfiguration*>(source.get());
        if (jsonData) {
            const QJsonObject& data = jsonData->data();
            for (auto it = data.begin(); it != data.end(); ++it) {
                merged.insert(it.key(), it.value());
            }
        }
    }

    QJsonDocument doc(merged);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return Error(ErrorCode::InternalError,
                     "Failed to open file for writing: " + filePath.toStdString());
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    SC_INFO("Config saved to: " + filePath.toStdString());
    return {};
}

Result<void> Config::saveAll() const {
    QMutexLocker lock(&m_dataMutex);
    if (m_configDir.isEmpty()) {
        return Error(ErrorCode::InternalError, "Config directory not set");
    }

    // 保存到配置目录下的默认文件
    // 注意: QRecursiveMutex 可重入, saveToFile 内部加锁安全
    QString defaultPath = m_configDir + "/config.json";
    return saveToFile(defaultPath);
}

void Config::setHotReloadEnabled(bool enabled) {
    QMutexLocker lock(&m_dataMutex);
    m_hotReloadEnabled = enabled;
}

bool Config::isHotReloadEnabled() const {
    QMutexLocker lock(&m_dataMutex);
    return m_hotReloadEnabled;
}

std::size_t Config::addChangeCallback(ConfigChangeCallback callback) {
    QMutexLocker lock(&m_dataMutex);
    std::size_t token = m_nextCallbackToken.fetch_add(1, std::memory_order_relaxed);
    m_changeCallbacks[token] = std::move(callback);
    return token;
}

bool Config::removeChangeCallback(std::size_t token) {
    QMutexLocker lock(&m_dataMutex);
    auto it = m_changeCallbacks.find(token);
    if (it == m_changeCallbacks.end()) {
        return false;
    }
    m_changeCallbacks.erase(it);
    return true;
}

bool Config::loadJsonFileLocked(const QString& filePath) {
    auto config = std::make_shared<JsonConfiguration>();
    if (config->load(filePath).isOk()) {
        m_configSources.push_back(config);
        SC_DEBUG("Loaded config file: " + filePath.toStdString());
        return true;
    }
    return false;
}

void Config::onFileChanged(const QString& path) {
    SC_INFO("Config file changed, reloading: " + path.toStdString());

    // 拷贝回调列表后在锁外触发,避免回调中再次访问 Config 造成死锁
    std::vector<ConfigChangeCallback> callbacksCopy;
    {
        QMutexLocker lock(&m_dataMutex);
        for (auto& source : m_configSources) {
            auto jsonConfig = dynamic_cast<JsonConfiguration*>(source.get());
            if (jsonConfig) {
                (void)jsonConfig->load(path);
            }
        }
        callbacksCopy.reserve(m_changeCallbacks.size());
        for (const auto& pair : m_changeCallbacks) {
            callbacksCopy.push_back(pair.second);
        }
    }

    for (const auto& callback : callbacksCopy) {
        callback(path);
    }
}

QVariant Config::getValueLocked(const QString& key) const {
    QString envVal = getEnvValue(key);
    if (!envVal.isNull()) {
        return QVariant(envVal);
    }

    for (auto it = m_configSources.rbegin(); it != m_configSources.rend(); ++it) {
        if ((*it)->contains(key)) {
            auto jsonConfig = dynamic_cast<const JsonConfiguration*>(it->get());
            if (jsonConfig) {
                const QJsonObject& data = jsonConfig->data();
                if (data.contains(key)) {
                    const QJsonValue& val = data[key];
                    if (val.isString()) return QVariant(val.toString());
                    if (val.isDouble()) return QVariant(val.toDouble());
                    if (val.isBool()) return QVariant(val.toBool());
                    if (val.isArray()) return QVariant(val.toArray().toVariantList());
                    if (val.isObject()) return QVariant(val.toObject().toVariantMap());
                }
            }
        }
    }
    return QVariant();
}

void Config::setValueLocked(const QString& key, const QVariant& value) {
    if (m_configSources.empty()) {
        auto config = std::make_shared<JsonConfiguration>();
        m_configSources.push_back(config);
    }

    auto source = m_configSources.back();
    if (value.typeId() == QMetaType::QString) {
        source->setString(key, value.toString());
    } else if (value.typeId() == QMetaType::Int || value.typeId() == QMetaType::LongLong) {
        source->setInt(key, value.toInt());
    } else if (value.typeId() == QMetaType::Double) {
        source->setDouble(key, value.toDouble());
    } else if (value.typeId() == QMetaType::Bool) {
        source->setBool(key, value.toBool());
    }

    // 回调交由调用方在锁外触发,避免回调中访问 Config 造成死锁
    // 调用方(setString/setInt/setDouble/setBool)负责拷贝回调列表并在锁外调用
}


void Config::setEnvPrefix(const QString& prefix) {
    QMutexLocker lock(&m_dataMutex);
    m_envPrefix = prefix;
}

QString Config::envPrefix() const {
    QMutexLocker lock(&m_dataMutex);
    return m_envPrefix;
}

// --- Profile 环境隔离(对标 SpringBoot application-{profile}.yml) ---

void Config::setProfile(const QString& profile) {
    QMutexLocker lock(&m_dataMutex);
    m_profile = profile;
    SC_INFO(QString("Config: profile set to '%1'").arg(profile).toStdString());
}

QString Config::profile() const {
    QMutexLocker lock(&m_dataMutex);
    return m_profile;
}

Result<void> Config::loadProfile(const QString& profile) {
    if (profile.isEmpty()) {
        return Error(ErrorCode::InvalidArgument, "Config: profile name cannot be empty");
    }

    QString configDir;
    {
        QMutexLocker lock(&m_dataMutex);
        m_profile = profile;
        configDir = m_configDir;
    }

    // 在已加载的配置目录中查找 application-{profile}.json
    if (configDir.isEmpty()) {
        SC_INFO("Config: no config directory loaded, profile file skip");
        return Ok();
    }

    QString profileFile = configDir + "/application-" + profile + ".json";
    QFileInfo fi(profileFile);
    if (!fi.exists()) {
        SC_INFO(QString("Config: profile file not found: %1, using base config only")
                    .arg(profileFile).toStdString());
        return Ok();
    }

    {
        QMutexLocker lock(&m_dataMutex);
        if (!loadJsonFileLocked(profileFile)) {
            return Error(ErrorCode::InternalError,
                         QString("Config: failed to load profile file: %1").arg(profileFile));
        }
    }

    SC_INFO(QString("Config: profile '%1' loaded from %2").arg(profile).arg(profileFile).toStdString());
    return Ok();
}

QVariantMap Config::getGroup(const QString& group) const {
    QMutexLocker lock(&m_dataMutex);
    QVariantMap result;
    QString prefix = group + ".";

    for (const auto& source : m_configSources) {
        auto jsonConfig = dynamic_cast<const JsonConfiguration*>(source.get());
        if (!jsonConfig) continue;
        const QJsonObject& data = jsonConfig->data();
        for (auto it = data.begin(); it != data.end(); ++it) {
            QString key = it.key();
            if (key.startsWith(prefix)) {
                QString subKey = key.mid(prefix.length());
                const QJsonValue& val = it.value();
                if (val.isString()) result[subKey] = val.toString();
                else if (val.isDouble()) result[subKey] = val.toDouble();
                else if (val.isBool()) result[subKey] = val.toBool();
                else if (val.isArray()) result[subKey] = val.toArray().toVariantList();
                else if (val.isObject()) result[subKey] = val.toObject().toVariantMap();
            }
        }
    }

    QString envPrefix = m_envPrefix + group.toUpper().replace(".", "_") + "_";
    QStringList keys = QProcessEnvironment::systemEnvironment().keys();
    for (const QString& envKey : keys) {
        if (envKey.startsWith(envPrefix)) {
            QString subKey = envKey.mid(envPrefix.length()).toLower().replace("_", ".");
            QString envValue = qEnvironmentVariable(envKey.toUtf8().constData());
            result[subKey] = envValue;
        }
    }

    return result;
}

QVariantMap Config::getAll() const {
    QMutexLocker lock(&m_dataMutex);
    QVariantMap result;

    for (const auto& source : m_configSources) {
        auto jsonConfig = dynamic_cast<const JsonConfiguration*>(source.get());
        if (!jsonConfig) continue;
        const QJsonObject& data = jsonConfig->data();
        for (auto it = data.begin(); it != data.end(); ++it) {
            QString key = it.key();
            const QJsonValue& val = it.value();
            if (val.isString()) result[key] = val.toString();
            else if (val.isDouble()) result[key] = val.toDouble();
            else if (val.isBool()) result[key] = val.toBool();
            else if (val.isArray()) result[key] = val.toArray().toVariantList();
            else if (val.isObject()) result[key] = val.toObject().toVariantMap();
        }
    }

    return result;
}

QString Config::getEnvKey(const QString& key) const {
    // 调用方已持锁(通过 getValueLocked → getEnvValue → getEnvKey),
    // QRecursiveMutex 可重入,这里加锁是防御性的,确保直接调用时也安全。
    QMutexLocker lock(&m_dataMutex);
    return m_envPrefix + key.toUpper().replace(".", "_");
}

QString Config::getEnvValue(const QString& key) const {
    // 注意:仅检查 isNull()(环境变量未设置),不检查 isEmpty()。
    // 用户显式将环境变量设为空字符串是有效语义,不应被当作"未设置"处理(新-4 修复)。
    QString envKey = getEnvKey(key);
    QByteArray envKeyBytes = envKey.toUtf8();
    const char* envKeyStr = envKeyBytes.constData();
    QString value = qEnvironmentVariable(envKeyStr);
    if (value.isNull()) {
        return QString();
    }
    return value;
}

bool Config::validate(const ConfigSchema& schema, QString* errorMsg) const {
    Q_UNUSED(schema);
    Q_UNUSED(errorMsg);
    return true;
}

}
