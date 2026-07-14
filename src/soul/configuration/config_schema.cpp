#include "soul/configuration/config_schema.h"

namespace sc {

ConfigSchema::ConfigSchema() {
}

void ConfigSchema::addField(const ConfigField& field) {
    m_fields.push_back(field);
}

bool ConfigSchema::validate(const QVariantMap& config, QString* errorMsg) const {
    for (const auto& field : m_fields) {
        if (field.required && !config.contains(field.name)) {
            if (errorMsg) {
                *errorMsg = QString("Missing required field: %1").arg(field.name);
            }
            return false;
        }

        if (config.contains(field.name)) {
            const QVariant& value = config[field.name];

            switch (field.type) {
            case ConfigType::String:
                if (!value.canConvert<QString>()) {
                    if (errorMsg) {
                        *errorMsg = QString("Field %1 must be a string").arg(field.name);
                    }
                    return false;
                }
                break;
            case ConfigType::Int:
                if (!value.canConvert<int>()) {
                    if (errorMsg) {
                        *errorMsg = QString("Field %1 must be an integer").arg(field.name);
                    }
                    return false;
                }
                break;
            case ConfigType::Double:
                if (!value.canConvert<double>()) {
                    if (errorMsg) {
                        *errorMsg = QString("Field %1 must be a double").arg(field.name);
                    }
                    return false;
                }
                break;
            case ConfigType::Bool:
                if (!value.canConvert<bool>()) {
                    if (errorMsg) {
                        *errorMsg = QString("Field %1 must be a boolean").arg(field.name);
                    }
                    return false;
                }
                break;
            case ConfigType::StringList:
                if (!value.canConvert<QStringList>()) {
                    if (errorMsg) {
                        *errorMsg = QString("Field %1 must be a string list").arg(field.name);
                    }
                    return false;
                }
                break;
            case ConfigType::IntList:
                if (!value.canConvert<QList<int>>()) {
                    if (errorMsg) {
                        *errorMsg = QString("Field %1 must be an integer list").arg(field.name);
                    }
                    return false;
                }
                break;
            case ConfigType::Object:
                if (!value.canConvert<QVariantMap>()) {
                    if (errorMsg) {
                        *errorMsg = QString("Field %1 must be an object").arg(field.name);
                    }
                    return false;
                }
                break;
            }

            if (field.validator && !field.validator(value)) {
                if (errorMsg) {
                    *errorMsg = QString("Field %1 validation failed").arg(field.name);
                }
                return false;
            }
        }
    }

    return true;
}

QVariantMap ConfigSchema::applyDefaults(const QVariantMap& config) const {
    QVariantMap result = config;

    for (const auto& field : m_fields) {
        if (!result.contains(field.name) && field.defaultValue.isValid()) {
            result[field.name] = field.defaultValue;
        }
    }

    return result;
}

const std::vector<ConfigField>& ConfigSchema::fields() const {
    return m_fields;
}

}
