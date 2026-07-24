#ifndef SOUL_CONFIGURATION_INI_CONFIGURATION_H
#define SOUL_CONFIGURATION_INI_CONFIGURATION_H

#include "iconfiguration.h"
#include <QSettings>
#include <memory>

namespace sc {

class IniConfiguration : public IConfiguration {
public:
    IniConfiguration();
    ~IniConfiguration() override = default;

    Result<void> load(const QString& filePath) override;
    Result<void> save(const QString& filePath) override;

    QString getString(const QString& key, const QString& defaultValue = "") const override;
    int getInt(const QString& key, int defaultValue = 0) const override;
    double getDouble(const QString& key, double defaultValue = 0.0) const override;
    bool getBool(const QString& key, bool defaultValue = false) const override;

    void setString(const QString& key, const QString& value) override;
    void setInt(const QString& key, int value) override;
    void setDouble(const QString& key, double value) override;
    void setBool(const QString& key, bool value) override;

    bool contains(const QString& key) const override;
    void remove(const QString& key) override;

private:
    std::unique_ptr<QSettings> m_settings;
    QString m_currentPath;
};

}

#endif
