#include "soul/configuration/ini_configuration.h"
#include "soul/core/error.h"

namespace sc {

IniConfiguration::IniConfiguration() {
}

Result<void> IniConfiguration::load(const QString& filePath) {
    m_settings = std::make_unique<QSettings>(filePath, QSettings::IniFormat);
    m_currentPath = filePath;
    return {};
}

Result<void> IniConfiguration::save(const QString& filePath) {
    Q_UNUSED(filePath);
    if (m_settings) {
        m_settings->sync();
    }
    return {};
}

QString IniConfiguration::getString(const QString& key, const QString& defaultValue) const {
    if (!m_settings) return defaultValue;
    return m_settings->value(key, defaultValue).toString();
}

int IniConfiguration::getInt(const QString& key, int defaultValue) const {
    if (!m_settings) return defaultValue;
    return m_settings->value(key, defaultValue).toInt();
}

double IniConfiguration::getDouble(const QString& key, double defaultValue) const {
    if (!m_settings) return defaultValue;
    return m_settings->value(key, defaultValue).toDouble();
}

bool IniConfiguration::getBool(const QString& key, bool defaultValue) const {
    if (!m_settings) return defaultValue;
    return m_settings->value(key, defaultValue).toBool();
}

void IniConfiguration::setString(const QString& key, const QString& value) {
    if (m_settings) {
        m_settings->setValue(key, value);
    }
}

void IniConfiguration::setInt(const QString& key, int value) {
    if (m_settings) {
        m_settings->setValue(key, value);
    }
}

void IniConfiguration::setDouble(const QString& key, double value) {
    if (m_settings) {
        m_settings->setValue(key, value);
    }
}

void IniConfiguration::setBool(const QString& key, bool value) {
    if (m_settings) {
        m_settings->setValue(key, value);
    }
}

bool IniConfiguration::contains(const QString& key) const {
    if (!m_settings) return false;
    return m_settings->contains(key);
}

void IniConfiguration::remove(const QString& key) {
    if (m_settings) {
        m_settings->remove(key);
    }
}

}
