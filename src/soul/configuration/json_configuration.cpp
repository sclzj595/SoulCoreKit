#include "soul/configuration/json_configuration.h"
#include "soul/core/error.h"
#include "soul/utils/json/json_helper.h"
#include <QFile>

namespace sc {

JsonConfiguration::JsonConfiguration() {
}

Result<void> JsonConfiguration::load(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Error(ErrorCode::NotFound, "Failed to open config file: " + filePath.toStdString());
    }
    QByteArray data = file.readAll();
    auto result = sc::json::deserialize(data);
    if (!result.isOk()) {
        return result.unwrapErr();
    }
    auto j = result.unwrap();
    if (!j.is_object()) {
        return Error(ErrorCode::ParseError, "Invalid JSON format in config file");
    }
    m_data = j;
    return {};
}

Result<void> JsonConfiguration::save(const QString& filePath) {
    return sc::json::saveToFilePretty(m_data, filePath);
}

QString JsonConfiguration::getString(const QString& key, const QString& defaultValue) const {
    return sc::json::getString(m_data, key.toStdString(), defaultValue);
}

int JsonConfiguration::getInt(const QString& key, int defaultValue) const {
    return sc::json::getInt(m_data, key.toStdString(), defaultValue);
}

double JsonConfiguration::getDouble(const QString& key, double defaultValue) const {
    return sc::json::getDouble(m_data, key.toStdString(), defaultValue);
}

bool JsonConfiguration::getBool(const QString& key, bool defaultValue) const {
    return sc::json::getBool(m_data, key.toStdString(), defaultValue);
}

void JsonConfiguration::setString(const QString& key, const QString& value) {
    m_data[key.toStdString()] = value.toStdString();
}

void JsonConfiguration::setInt(const QString& key, int value) {
    m_data[key.toStdString()] = value;
}

void JsonConfiguration::setDouble(const QString& key, double value) {
    m_data[key.toStdString()] = value;
}

void JsonConfiguration::setBool(const QString& key, bool value) {
    m_data[key.toStdString()] = value;
}

bool JsonConfiguration::contains(const QString& key) const {
    return m_data.contains(key.toStdString());
}

void JsonConfiguration::remove(const QString& key) {
    m_data.erase(key.toStdString());
}

const sc::json::Json& JsonConfiguration::data() const {
    return m_data;
}

}
