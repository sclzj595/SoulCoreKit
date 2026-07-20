#include "soul/configuration/config.h"
#include "soul/configuration/json_configuration.h"
#include "soul/logging/log_macros.h"
#include "soul/core/error.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QVariant>

namespace sc {

Config::Config() {
    QObject::connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged,
                     this, &Config::onFileChanged);
}

void Config::init() {
}

void Config::shutdown() {
    saveAll();
}

Config::~Config() {
}

Result<void> Config::loadFromDirectory(const QString& configDir) {
    m_configDir = configDir;

    QDir dir(configDir);
    if (!dir.exists()) {
        SC_WARN("Config directory does not exist: " + configDir.toStdString());
        return Error(ErrorCode::NotFound, "Config directory does not exist: " + configDir.toStdString());
    }

    QStringList filters;
    filters << "*.json";
    QStringList files = dir.entryList(filters, QDir::Files);

    for (const QString& file : files) {
        loadJsonFile(dir.filePath(file));
    }

    if (isHotReloadEnabled()) {
        for (const QString& file : files) {
            m_fileWatcher.addPath(dir.filePath(file));
        }
    }

    SC_INFO("Loaded config from directory: " + configDir.toStdString());
    return {};
}

Result<void> Config::loadFile(const QString& filePath) {
    if (loadJsonFile(filePath)) {
        return {};
    }
    return Error(ErrorCode::InternalError, "Failed to load config file: " + filePath.toStdString());
}

Result<void> Config::loadEnvironment(const QString& env) {
    m_currentEnv = env;

    QString envDir = QDir(m_configDir).filePath(env);
    QDir dir(envDir);

    if (!dir.exists()) {
        SC_WARN("Environment directory does not exist: " + envDir.toStdString());
        return Error(ErrorCode::NotFound, "Environment directory does not exist: " + envDir.toStdString());
    }

    QStringList filters;
    filters << "*.json";
    QStringList files = dir.entryList(filters, QDir::Files);

    for (const QString& file : files) {
        loadJsonFile(dir.filePath(file));
    }

    if (isHotReloadEnabled()) {
        for (const QString& file : files) {
            m_fileWatcher.addPath(dir.filePath(file));
        }
    }

    SC_INFO("Loaded environment config: " + env.toStdString());
    return {};
}

QString Config::getString(const QString& key, const QString& defaultValue) const {
    QVariant value = getValue(key);
    if (value.isValid()) {
        return value.toString();
    }
    return defaultValue;
}

int Config::getInt(const QString& key, int defaultValue) const {
    QVariant value = getValue(key);
    if (value.isValid()) {
        return value.toInt();
    }
    return defaultValue;
}

double Config::getDouble(const QString& key, double defaultValue) const {
    QVariant value = getValue(key);
    if (value.isValid()) {
        return value.toDouble();
    }
    return defaultValue;
}

bool Config::getBool(const QString& key, bool defaultValue) const {
    QVariant value = getValue(key);
    if (value.isValid()) {
        return value.toBool();
    }
    return defaultValue;
}

void Config::setString(const QString& key, const QString& value) {
    setValue(key, value);
}

void Config::setInt(const QString& key, int value) {
    setValue(key, value);
}

void Config::setDouble(const QString& key, double value) {
    setValue(key, value);
}

void Config::setBool(const QString& key, bool value) {
    setValue(key, value);
}

bool Config::contains(const QString& key) const {
    return getValue(key).isValid();
}

void Config::remove(const QString& key) {
    for (auto& source : m_configSources) {
        source->remove(key);
    }
}

Result<void> Config::saveToFile(const QString& filePath) const {
    if (m_configSources.empty()) {
        return Error(ErrorCode::InternalError, "No config sources to save");
    }

    auto jsonConfig = std::make_shared<JsonConfiguration>();
    for (const auto& source : m_configSources) {
        const auto& jsonData = dynamic_cast<const JsonConfiguration*>(source.get());
        if (jsonData) {
            const QJsonObject& data = jsonData->data();
            for (auto it = data.begin(); it != data.end(); ++it) {
                Q_UNUSED(it);
            }
        }
    }

    return jsonConfig->save(filePath);
}

Result<void> Config::saveAll() const {
    if (m_configDir.isEmpty()) {
        return Error(ErrorCode::InternalError, "Config directory not set");
    }

    return {};
}

void Config::setHotReloadEnabled(bool enabled) {
    m_hotReloadEnabled = enabled;
}

bool Config::isHotReloadEnabled() const {
    return m_hotReloadEnabled;
}

void Config::addChangeCallback(ConfigChangeCallback callback) {
    m_changeCallbacks.push_back(std::move(callback));
}

void Config::removeChangeCallback(ConfigChangeCallback callback) {
    auto it = std::remove_if(m_changeCallbacks.begin(), m_changeCallbacks.end(),
                             [&](const ConfigChangeCallback& cb) {
                                 return &cb == &callback;
                             });
    m_changeCallbacks.erase(it, m_changeCallbacks.end());
}

bool Config::loadJsonFile(const QString& filePath) {
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

    for (auto& source : m_configSources) {
        auto jsonConfig = dynamic_cast<JsonConfiguration*>(source.get());
        if (jsonConfig) {
            jsonConfig->load(path);
        }
    }

    for (const auto& callback : m_changeCallbacks) {
        callback(path);
    }
}

QVariant Config::getValue(const QString& key) const {
    for (auto it = m_configSources.rbegin(); it != m_configSources.rend(); ++it) {
        if ((*it)->contains(key)) {
            QVariant value;
            if ((*it)->getString(key) != "") {
                value = (*it)->getString(key);
            } else if ((*it)->getInt(key, 0) != 0) {
                value = (*it)->getInt(key);
            } else if ((*it)->getDouble(key, 0.0) != 0.0) {
                value = (*it)->getDouble(key);
            } else if ((*it)->getBool(key, false)) {
                value = (*it)->getBool(key);
            }
            return value;
        }
    }
    return QVariant();
}

void Config::setValue(const QString& key, const QVariant& value) {
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

    for (const auto& callback : m_changeCallbacks) {
        callback(key);
    }
}


void Config::setEnvPrefix(const QString& prefix) {
    m_envPrefix = prefix;
}

QString Config::envPrefix() const {
    return m_envPrefix;
}

QVariantMap Config::getGroup(const QString& group) const {
    Q_UNUSED(group);
    return QVariantMap();
}

QVariantMap Config::getAll() const {
    return QVariantMap();
}

bool Config::validate(const ConfigSchema& schema, QString* errorMsg) const {
    Q_UNUSED(schema);
    Q_UNUSED(errorMsg);
    return true;
}

}
