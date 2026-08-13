#include "soul/configuration/config.h"
#include "soul/configuration/json_configuration.h"
#include "soul/configuration/config_schema.h"  // v2.9.0: validate() 实现
#include "soul/logging/log_macros.h"
#include "soul/core/error.h"
#include "soul/utils/json/json_helper.h"
#include <memory>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

    // 合并所有配置源的数据到单个 sc::json::Json
    sc::json::Json merged = sc::json::Json::object();
    for (const auto& source : m_configSources) {
        auto jsonConfig = dynamic_cast<const JsonConfiguration*>(source.get());
        if (jsonConfig) {
            const sc::json::Json& data = jsonConfig->data();
            for (auto it = data.begin(); it != data.end(); ++it) {
                merged[it.key()] = it.value();
            }
        }
    }

    auto result = sc::json::saveToFilePretty(merged, filePath);
    if (result.isErr()) {
        return result.unwrapErr();
    }

    SC_INFO("Config saved to: " + filePath.toStdString());
    return {};
}

Result<void> Config::saveAll() const {
    QMutexLocker lock(&m_dataMutex);
    if (m_configDir.isEmpty()) {
        return Error(ErrorCode::InternalError, "Config directory not set");
    }

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
                const sc::json::Json& data = jsonConfig->data();
                std::string stdKey = key.toStdString();
                if (data.contains(stdKey)) {
                    const sc::json::Json& val = data[stdKey];
                    if (val.is_string()) return QVariant(QString::fromStdString(val.get<std::string>()));
                    if (val.is_number_integer()) return QVariant(static_cast<qint64>(val.get<int64_t>()));
                    if (val.is_number_float()) return QVariant(val.get<double>());
                    if (val.is_boolean()) return QVariant(val.get<bool>());
                    if (val.is_array()) {
                        QVariantList list;
                        for (const auto& v : val) {
                            if (v.is_string()) list << QString::fromStdString(v.get<std::string>());
                            else if (v.is_number_integer()) list << static_cast<qint64>(v.get<int64_t>());
                            else if (v.is_number_float()) list << v.get<double>();
                            else if (v.is_boolean()) list << v.get<bool>();
                            else list << QString::fromStdString(v.dump());
                        }
                        return QVariant(list);
                    }
                    if (val.is_object()) {
                        QVariantMap map;
                        for (auto vit = val.begin(); vit != val.end(); ++vit) {
                            const sc::json::Json& vv = vit.value();
                            if (vv.is_string()) map[QString::fromStdString(vit.key())] = QString::fromStdString(vv.get<std::string>());
                            else if (vv.is_number_integer()) map[QString::fromStdString(vit.key())] = static_cast<qint64>(vv.get<int64_t>());
                            else if (vv.is_number_float()) map[QString::fromStdString(vit.key())] = vv.get<double>();
                            else if (vv.is_boolean()) map[QString::fromStdString(vit.key())] = vv.get<bool>();
                        }
                        return QVariant(map);
                    }
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
}


void Config::setEnvPrefix(const QString& prefix) {
    QMutexLocker lock(&m_dataMutex);
    m_envPrefix = prefix;
}

QString Config::envPrefix() const {
    QMutexLocker lock(&m_dataMutex);
    return m_envPrefix;
}

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
        const sc::json::Json& data = jsonConfig->data();
        for (auto it = data.begin(); it != data.end(); ++it) {
            QString key = QString::fromStdString(it.key());
            if (key.startsWith(prefix)) {
                QString subKey = key.mid(prefix.length());
                const sc::json::Json& val = it.value();
                if (val.is_string()) result[subKey] = QString::fromStdString(val.get<std::string>());
                else if (val.is_number_integer()) result[subKey] = static_cast<qint64>(val.get<int64_t>());
                else if (val.is_number_float()) result[subKey] = val.get<double>();
                else if (val.is_boolean()) result[subKey] = val.get<bool>();
                else if (val.is_array()) {
                    QVariantList list;
                    for (const auto& v : val) {
                        if (v.is_string()) list << QString::fromStdString(v.get<std::string>());
                        else if (v.is_number_integer()) list << static_cast<qint64>(v.get<int64_t>());
                        else if (v.is_number_float()) list << v.get<double>();
                        else if (v.is_boolean()) list << v.get<bool>();
                    }
                    result[subKey] = list;
                }
                else if (val.is_object()) {
                    QVariantMap map;
                    for (auto vit = val.begin(); vit != val.end(); ++vit) {
                        const sc::json::Json& vv = vit.value();
                        if (vv.is_string()) map[QString::fromStdString(vit.key())] = QString::fromStdString(vv.get<std::string>());
                        else if (vv.is_number_integer()) map[QString::fromStdString(vit.key())] = static_cast<qint64>(vv.get<int64_t>());
                        else if (vv.is_number_float()) map[QString::fromStdString(vit.key())] = vv.get<double>();
                        else if (vv.is_boolean()) map[QString::fromStdString(vit.key())] = vv.get<bool>();
                    }
                    result[subKey] = map;
                }
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
        const sc::json::Json& data = jsonConfig->data();
        for (auto it = data.begin(); it != data.end(); ++it) {
            QString key = QString::fromStdString(it.key());
            const sc::json::Json& val = it.value();
            if (val.is_string()) result[key] = QString::fromStdString(val.get<std::string>());
            else if (val.is_number_integer()) result[key] = static_cast<qint64>(val.get<int64_t>());
            else if (val.is_number_float()) result[key] = val.get<double>();
            else if (val.is_boolean()) result[key] = val.get<bool>();
            else if (val.is_array()) {
                QVariantList list;
                for (const auto& v : val) {
                    if (v.is_string()) list << QString::fromStdString(v.get<std::string>());
                    else if (v.is_number_integer()) list << static_cast<qint64>(v.get<int64_t>());
                    else if (v.is_number_float()) list << v.get<double>();
                    else if (v.is_boolean()) list << v.get<bool>();
                }
                result[key] = list;
            }
            else if (val.is_object()) {
                QVariantMap map;
                for (auto vit = val.begin(); vit != val.end(); ++vit) {
                    const sc::json::Json& vv = vit.value();
                    if (vv.is_string()) map[QString::fromStdString(vit.key())] = QString::fromStdString(vv.get<std::string>());
                    else if (vv.is_number_integer()) map[QString::fromStdString(vit.key())] = static_cast<qint64>(vv.get<int64_t>());
                    else if (vv.is_number_float()) map[QString::fromStdString(vit.key())] = vv.get<double>();
                    else if (vv.is_boolean()) map[QString::fromStdString(vit.key())] = vv.get<bool>();
                }
                result[key] = map;
            }
        }
    }

    return result;
}

QString Config::getEnvKey(const QString& key) const {
    QMutexLocker lock(&m_dataMutex);
    return m_envPrefix + key.toUpper().replace(".", "_");
}

QString Config::getEnvValue(const QString& key) const {
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
    // v2.9.0: 实际调用 ConfigSchema::validate()
    auto allValues = getAll();
    QVariantMap map;
    for (auto it = allValues.begin(); it != allValues.end(); ++it) {
        map.insert(it.key(), it.value());
    }
    return schema.validate(map, errorMsg);
}

}
