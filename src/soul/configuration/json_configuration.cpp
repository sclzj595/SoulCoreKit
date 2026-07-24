#include "soul/configuration/json_configuration.h"
#include "soul/core/error.h"
#include <QJsonDocument>
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
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return Error(ErrorCode::ParseError, "Invalid JSON format in config file");
    }
    m_data = doc.object();
    return {};
}

Result<void> JsonConfiguration::save(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return Error(ErrorCode::InternalError, "Failed to open config file for writing: " + filePath.toStdString());
    }
    QJsonDocument doc(m_data);
    file.write(doc.toJson(QJsonDocument::Indented));
    return {};
}

QString JsonConfiguration::getString(const QString& key, const QString& defaultValue) const {
    if (m_data.contains(key) && m_data[key].isString()) {
        return m_data[key].toString();
    }
    return defaultValue;
}

int JsonConfiguration::getInt(const QString& key, int defaultValue) const {
    if (m_data.contains(key) && m_data[key].isDouble()) {
        return m_data[key].toInt();
    }
    return defaultValue;
}

double JsonConfiguration::getDouble(const QString& key, double defaultValue) const {
    if (m_data.contains(key) && m_data[key].isDouble()) {
        return m_data[key].toDouble();
    }
    return defaultValue;
}

bool JsonConfiguration::getBool(const QString& key, bool defaultValue) const {
    if (m_data.contains(key) && m_data[key].isBool()) {
        return m_data[key].toBool();
    }
    return defaultValue;
}

void JsonConfiguration::setString(const QString& key, const QString& value) {
    m_data[key] = value;
}

void JsonConfiguration::setInt(const QString& key, int value) {
    m_data[key] = value;
}

void JsonConfiguration::setDouble(const QString& key, double value) {
    m_data[key] = value;
}

void JsonConfiguration::setBool(const QString& key, bool value) {
    m_data[key] = value;
}

bool JsonConfiguration::contains(const QString& key) const {
    return m_data.contains(key);
}

void JsonConfiguration::remove(const QString& key) {
    m_data.remove(key);
}

const QJsonObject& JsonConfiguration::data() const {
    return m_data;
}

}
