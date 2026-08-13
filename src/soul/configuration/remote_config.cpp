#include "soul/configuration/remote_config.h"
#include "soul/core/error.h"
#include "soul/utils/json/json_helper.h"
#include <QFile>

namespace sc {

RemoteConfiguration::RemoteConfiguration(QObject* parent)
    : QObject(parent)
{
}

void RemoteConfiguration::setSource(std::unique_ptr<IRemoteConfigSource> source) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_source = std::move(source);
}

Result<void> RemoteConfiguration::sync(const QString& namespaceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_source) {
        return Error(ErrorCode::NotConnected, "No remote config source set");
    }
    auto result = m_source->fetchConfig(namespaceName);
    if (result.isErr()) {
        return result.unwrapErr();
    }
    m_config = result.unwrap();
    emit configSynced(namespaceName);
    return Ok();
}

Result<void> RemoteConfiguration::watch(const QString& namespaceName) {
    IRemoteConfigSource* source = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_source) {
            return Error(ErrorCode::NotConnected, "No remote config source set");
        }
        source = m_source.get();
    }
    return source->watchConfig(namespaceName, [this](const sc::json::Json& config) {
        std::lock_guard<std::mutex> lock2(m_mutex);
        m_config = config;
        emit configSynced("");
    });
}

Result<void> RemoteConfiguration::load(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Error(ErrorCode::NotFound,
                     "Failed to open file: " + filePath.toStdString());
    }
    auto result = sc::json::deserialize(file.readAll());
    if (result.isErr()) {
        return result.unwrapErr();
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config = result.unwrap();
    }
    return Ok();
}

Result<void> RemoteConfiguration::save(const QString& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return sc::json::saveToFilePretty(m_config, filePath);
}

QString RemoteConfiguration::getString(const QString& key, const QString& defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return sc::json::getString(m_config, key.toStdString(), defaultValue);
}

int RemoteConfiguration::getInt(const QString& key, int defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return sc::json::getInt(m_config, key.toStdString(), defaultValue);
}

double RemoteConfiguration::getDouble(const QString& key, double defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return sc::json::getDouble(m_config, key.toStdString(), defaultValue);
}

bool RemoteConfiguration::getBool(const QString& key, bool defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return sc::json::getBool(m_config, key.toStdString(), defaultValue);
}

void RemoteConfiguration::setString(const QString& key, const QString& value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config[key.toStdString()] = value.toStdString();
    }
    emit configChanged(key, value);
}

void RemoteConfiguration::setInt(const QString& key, int value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config[key.toStdString()] = value;
    }
    emit configChanged(key, QString::number(value));
}

void RemoteConfiguration::setDouble(const QString& key, double value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config[key.toStdString()] = value;
    }
    emit configChanged(key, QString::number(value));
}

void RemoteConfiguration::setBool(const QString& key, bool value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config[key.toStdString()] = value;
    }
    emit configChanged(key, value ? QString("true") : QString("false"));
}

bool RemoteConfiguration::contains(const QString& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return sc::json::contains(m_config, key.toStdString());
}

void RemoteConfiguration::remove(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.erase(key.toStdString());
}

} // namespace sc
