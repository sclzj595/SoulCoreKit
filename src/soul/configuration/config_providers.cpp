// ============================================================================
// config_providers.cpp — 内置 Provider 实现 [v2.9.0]
// ============================================================================

#include "soul/configuration/config_providers.h"
#include "soul/configuration/remote_config.h"
#include <QProcessEnvironment>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

namespace sc {

// ============================================================================
// JsonFileConfigProvider
// ============================================================================

JsonFileConfigProvider::JsonFileConfigProvider(const QString& filePath, int prio)
    : m_filePath(filePath), m_priority(prio) {}

Result<ConfigSnapshot> JsonFileConfigProvider::load() {
    if (!QFileInfo::exists(m_filePath)) {
        return Result<ConfigSnapshot>::err(
            Error(ErrorCode::FileNotFound,
                QString("Config file not found: %1").arg(m_filePath))
        );
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<ConfigSnapshot>::err(
            Error(ErrorCode::FileReadError,
                QString("Cannot open: %1").arg(m_filePath))
        );
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError) {
        return Result<ConfigSnapshot>::err(
            Error(ErrorCode::ParseError,
                QString("JSON parse error: %1").arg(parseError.errorString()))
        );
    }

    QHash<QString, QVariant> values;
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            values.insert(it.key(), it.value().toVariant());
        }
    }

    ConfigSnapshot snap(std::move(values));
    snap.setSourceName("JsonFile:" + m_filePath);
    snap.setLoadedAt(std::chrono::system_clock::now());
    return Result<ConfigSnapshot>::ok(std::move(snap));
}

// ============================================================================
// IniFileConfigProvider
// ============================================================================

IniFileConfigProvider::IniFileConfigProvider(const QString& filePath, int prio)
    : m_filePath(filePath), m_priority(prio) {}

Result<ConfigSnapshot> IniFileConfigProvider::load() {
    if (!QFileInfo::exists(m_filePath)) {
        return Result<ConfigSnapshot>::err(
            Error(ErrorCode::FileNotFound,
                QString("Config file not found: %1").arg(m_filePath))
        );
    }

    QSettings settings(m_filePath, QSettings::IniFormat);
    QHash<QString, QVariant> values;
    for (const auto& key : settings.allKeys()) {
        values.insert(key, settings.value(key));
    }

    ConfigSnapshot snap(std::move(values));
    snap.setSourceName("IniFile:" + m_filePath);
    snap.setLoadedAt(std::chrono::system_clock::now());
    return Result<ConfigSnapshot>::ok(std::move(snap));
}

// ============================================================================
// EnvironmentConfigProvider
// ============================================================================

EnvironmentConfigProvider::EnvironmentConfigProvider(const QString& prefix, int prio)
    : m_prefix(prefix.toUpper()), m_priority(prio) {}

Result<ConfigSnapshot> EnvironmentConfigProvider::load() {
    QHash<QString, QVariant> values;
    auto env = QProcessEnvironment::systemEnvironment();
    const auto keys = env.keys();

    for (const auto& envKey : keys) {
        if (envKey.startsWith(m_prefix, Qt::CaseInsensitive)) {
            auto configKey = envKeyToConfigKey(m_prefix, envKey);
            values.insert(configKey, QVariant(env.value(envKey)));
        }
    }

    ConfigSnapshot snap(std::move(values));
    snap.setSourceName("Environment");
    snap.setLoadedAt(std::chrono::system_clock::now());
    return Result<ConfigSnapshot>::ok(std::move(snap));
}

QString EnvironmentConfigProvider::envKeyToConfigKey(
    const QString& prefix, const QString& envKey) {
    // SOUL_SERVER_PORT → server.port
    // SOUL_DB_HOST → db.host
    QString key = envKey.mid(prefix.length());
    return key.toLower().replace('_', '.');
}

// ============================================================================
// CommandLineConfigProvider
// ============================================================================

CommandLineConfigProvider::CommandLineConfigProvider(int argc, char* argv[], int prio)
    : m_priority(prio) {
    for (int i = 1; i < argc; ++i) {
        m_args.push_back(argv[i]);
    }
}

Result<ConfigSnapshot> CommandLineConfigProvider::load() {
    QHash<QString, QVariant> values;

    for (const auto& arg : m_args) {
        auto [key, value] = parseArg(arg);
        if (!key.isEmpty()) {
            values.insert(key, QVariant(value));
        }
    }

    ConfigSnapshot snap(std::move(values));
    snap.setSourceName("CommandLine");
    snap.setLoadedAt(std::chrono::system_clock::now());
    return Result<ConfigSnapshot>::ok(std::move(snap));
}

std::pair<QString, QString> CommandLineConfigProvider::parseArg(
    const std::string& arg) {
    QString qarg = QString::fromStdString(arg);

    // --key=value
    if (qarg.startsWith("--")) {
        qarg = qarg.mid(2);
        int eqPos = qarg.indexOf('=');
        if (eqPos > 0) {
            return {qarg.left(eqPos), qarg.mid(eqPos + 1)};
        }
        // --flag (bool true)
        return {qarg, "true"};
    }

    // -k value (简化形式，暂不支持)
    return {};
}

// ============================================================================
// DefaultConfigProvider
// ============================================================================

DefaultConfigProvider::DefaultConfigProvider() = default;

void DefaultConfigProvider::setDefault(const QString& key, const QVariant& value) {
    m_defaults.insert(key, value);
}

Result<ConfigSnapshot> DefaultConfigProvider::load() {
    ConfigSnapshot snap(m_defaults);
    snap.setSourceName("Default");
    snap.setLoadedAt(std::chrono::system_clock::now());
    return Result<ConfigSnapshot>::ok(std::move(snap));
}

// ============================================================================
// RemoteConfigProvider
// ============================================================================

RemoteConfigProvider::RemoteConfigProvider(
    std::shared_ptr<IRemoteConfigSource> source,
    const QString& namespaceName, int prio)
    : m_source(std::move(source)), m_namespace(namespaceName), m_priority(prio) {}

Result<ConfigSnapshot> RemoteConfigProvider::load() {
    if (!m_source) {
        return Result<ConfigSnapshot>::err(
            Error(ErrorCode::InvalidState, "RemoteConfigProvider: source is null")
        );
    }

    auto connectResult = m_source->connectToServer();
    if (connectResult.isErr()) {
        return Result<ConfigSnapshot>::err(
            Error(ErrorCode::NetworkError,
                QString("Failed to connect to remote config: %1")
                    .arg(connectResult.unwrapErr().message()),
                std::make_shared<Error>(connectResult.unwrapErr()))
        );
    }

    auto fetchResult = m_source->fetchConfig(m_namespace);
    if (fetchResult.isErr()) {
        m_source->disconnectFromServer();
        return Result<ConfigSnapshot>::err(
            Error(ErrorCode::NetworkError,
                QString("Failed to fetch config '%1': %2")
                    .arg(m_namespace)
                    .arg(fetchResult.unwrapErr().message()),
                std::make_shared<Error>(fetchResult.unwrapErr()))
        );
    }

    // fetchConfig 返回 nlohmann::json (QHash-like map)
    auto rawConfig = fetchResult.unwrap();
    QHash<QString, QVariant> values;
    for (auto it = rawConfig.begin(); it != rawConfig.end(); ++it) {
        values.insert(QString::fromStdString(it.key()),
                      QVariant(QString::fromStdString(it.value().get<std::string>())));
    }

    m_source->disconnectFromServer();

    ConfigSnapshot snap(std::move(values));
    snap.setSourceName("Remote:" + m_namespace);
    snap.setLoadedAt(std::chrono::system_clock::now());
    return Result<ConfigSnapshot>::ok(std::move(snap));
}

} // namespace sc
