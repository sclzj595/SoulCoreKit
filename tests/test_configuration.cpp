#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>
#include <QVariantList>
#include <QSettings>
#include <QProcessEnvironment>
#include <QStringList>
#include <QList>
#include <QByteArray>
#include <memory>
#include <functional>

#include "soul/configuration/config.h"
#include "soul/configuration/json_configuration.h"
#include "soul/configuration/ini_configuration.h"
#include "soul/configuration/config_schema.h"

class TestConfiguration : public QObject {
    Q_OBJECT

private slots:
    // ===== Config: get with defaults =====
    void testConfigDefaults();
    void testConfigSetGetString();
    void testConfigSetGetInt();
    void testConfigSetGetDouble();
    void testConfigSetGetBool();

    // ===== Config: contains and remove =====
    void testConfigContains();
    void testConfigRemove();

    // ===== Config: getGroup and getAll =====
    void testConfigGetGroup();
    void testConfigGetAll();

    // ===== Config: validate =====
    void testConfigValidate();

    // ===== Config: environment variable override =====
    void testConfigEnvOverride();
    void testConfigEnvPrefix();

    // ===== Config: load from file =====
    void testConfigLoadFile();
    void testConfigLoadFileNonExistent();
    void testConfigLoadFileInvalidJson();

    // ===== Config: load from directory =====
    void testConfigLoadFromDirectory();
    void testConfigLoadFromDirectoryNonExistent();

    // ===== Config: load environment =====
    void testConfigLoadEnvironment();
    void testConfigLoadEnvironmentNonExistent();

    // ===== Config: save =====
    void testConfigSaveToFile();
    void testConfigSaveAll();

    // ===== Config: hot reload =====
    void testConfigHotReload();

    // ===== Config: change callback =====
    void testConfigChangeCallback();

    // ===== JsonConfiguration =====
    void testJsonLoadFromFile();
    void testJsonLoadNonExistent();
    void testJsonLoadInvalidJson();
    void testJsonSave();
    void testJsonGetValues();
    void testJsonSetValues();
    void testJsonContains();
    void testJsonRemove();
    void testJsonDefaults();
    void testJsonDataAccessor();

    // ===== IniConfiguration =====
    void testIniLoadFromFile();
    void testIniSave();
    void testIniGetSetValues();
    void testIniContains();
    void testIniRemove();

    // ===== ConfigSchema =====
    void testSchemaAddField();
    void testSchemaValidateRequired();
    void testSchemaValidateTypes();
    void testSchemaValidateCustomValidator();
    void testSchemaApplyDefaults();
    void testSchemaValidateOptionalField();
    void testEmptyConfigSchema();

    // ===== Edge cases =====
    void testMissingKeysReturnDefaults();
    void testInvalidJsonType();
    void testEnvVarOverrideEdgeCases();
};

// =========================================================================
// Config tests
// =========================================================================

void TestConfiguration::testConfigDefaults() {
    sc::Config& config = sc::Config::instance();
    QCOMPARE(config.getString("testcfg.missing.string", "fallback"), QString("fallback"));
    QCOMPARE(config.getInt("testcfg.missing.int", 99), 99);
    QCOMPARE(config.getDouble("testcfg.missing.double", 2.5), 2.5);
    QVERIFY(config.getBool("testcfg.missing.bool", true));
}

void TestConfiguration::testConfigSetGetString() {
    sc::Config& config = sc::Config::instance();
    config.setString("testcfg.setstr.key", "hello world");
    QCOMPARE(config.getString("testcfg.setstr.key", ""), QString("hello world"));
}

void TestConfiguration::testConfigSetGetInt() {
    sc::Config& config = sc::Config::instance();
    config.setInt("testcfg.setint.key", 12345);
    QCOMPARE(config.getInt("testcfg.setint.key", 0), 12345);
}

void TestConfiguration::testConfigSetGetDouble() {
    sc::Config& config = sc::Config::instance();
    config.setDouble("testcfg.setdouble.key", 3.14159);
    QCOMPARE(config.getDouble("testcfg.setdouble.key", 0.0), 3.14159);
}

void TestConfiguration::testConfigSetGetBool() {
    sc::Config& config = sc::Config::instance();
    config.setBool("testcfg.setbool.key", true);
    QVERIFY(config.getBool("testcfg.setbool.key", false));
}

void TestConfiguration::testConfigContains() {
    sc::Config& config = sc::Config::instance();
    config.setString("testcfg.contains.key", "value");
    QVERIFY(config.contains("testcfg.contains.key"));
    QVERIFY(!config.contains("testcfg.contains.nonexistent"));
}

void TestConfiguration::testConfigRemove() {
    sc::Config& config = sc::Config::instance();
    config.setString("testcfg.remove.key", "tempvalue");
    QVERIFY(config.contains("testcfg.remove.key"));
    config.remove("testcfg.remove.key");
    QVERIFY(!config.contains("testcfg.remove.key"));
}

void TestConfiguration::testConfigGetGroup() {
    sc::Config& config = sc::Config::instance();
    config.setString("testcfggrp.host", "localhost");
    config.setInt("testcfggrp.port", 3306);
    config.setBool("testcfggrp.enabled", true);

    QVariantMap group = config.getGroup("testcfggrp");
    QVERIFY(group.contains("host"));
    QCOMPARE(group.value("host").toString(), QString("localhost"));
    QVERIFY(group.contains("port"));
    QCOMPARE(group.value("port").toInt(), 3306);
    QVERIFY(group.contains("enabled"));
    QVERIFY(group.value("enabled").toBool());
}

void TestConfiguration::testConfigGetAll() {
    sc::Config& config = sc::Config::instance();
    config.setString("testcfgall.key1", "val1");
    config.setInt("testcfgall.key2", 42);

    QVariantMap all = config.getAll();
    QVERIFY(all.contains("testcfgall.key1"));
    QCOMPARE(all.value("testcfgall.key1").toString(), QString("val1"));
    QVERIFY(all.contains("testcfgall.key2"));
    QCOMPARE(all.value("testcfgall.key2").toInt(), 42);
}

void TestConfiguration::testConfigValidate() {
    sc::Config& config = sc::Config::instance();
    config.setString("testcfgvalidate.host", "localhost");
    config.setInt("testcfgvalidate.port", 8080);

    sc::ConfigSchema schema;

    sc::ConfigField hostField;
    hostField.name = "testcfgvalidate.host";
    hostField.type = sc::ConfigType::String;
    hostField.required = true;
    schema.addField(hostField);

    sc::ConfigField portField;
    portField.name = "testcfgvalidate.port";
    portField.type = sc::ConfigType::Int;
    portField.required = true;
    schema.addField(portField);

    QString errorMsg;
    QVERIFY(config.validate(schema, &errorMsg));
}

void TestConfiguration::testConfigEnvOverride() {
    sc::Config& config = sc::Config::instance();
    config.setEnvPrefix("SOUL_");

    // Set a config value
    config.setInt("testcfg.envoverride", 8080);
    QCOMPARE(config.getInt("testcfg.envoverride", 0), 8080);

    // Env var should override the config value
    qputenv("SOUL_TESTCFG_ENVOVERRIDE", "9090");
    QCOMPARE(config.getInt("testcfg.envoverride", 0), 9090);

    // After unsetting env var, config value should be used again
    qunsetenv("SOUL_TESTCFG_ENVOVERRIDE");
    QCOMPARE(config.getInt("testcfg.envoverride", 0), 8080);
}

void TestConfiguration::testConfigEnvPrefix() {
    sc::Config& config = sc::Config::instance();
    QString oldPrefix = config.envPrefix();

    config.setEnvPrefix("MYAPP_");
    QCOMPARE(config.envPrefix(), QString("MYAPP_"));

    // Key "testcfg.prefix" with prefix "MYAPP_" -> env var "MYAPP_TESTCFG_PREFIX"
    qputenv("MYAPP_TESTCFG_PREFIX", "envval");
    QCOMPARE(config.getString("testcfg.prefix", ""), QString("envval"));
    qunsetenv("MYAPP_TESTCFG_PREFIX");

    // Restore original prefix
    config.setEnvPrefix(oldPrefix);
}

void TestConfiguration::testConfigLoadFile() {
    QTemporaryFile file;
    file.open();
    file.write(R"({"testcfg.loadfile.host":"localhost","testcfg.loadfile.port":9090})");
    file.close();

    sc::Config& config = sc::Config::instance();
    auto result = config.loadFile(file.fileName());
    QVERIFY(result.isOk());
    QCOMPARE(config.getString("testcfg.loadfile.host", ""), QString("localhost"));
    QCOMPARE(config.getInt("testcfg.loadfile.port", 0), 9090);
}

void TestConfiguration::testConfigLoadFileNonExistent() {
    sc::Config& config = sc::Config::instance();
    auto result = config.loadFile("/nonexistent/path/to/file.json");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), sc::ErrorCode::InternalError);
}

void TestConfiguration::testConfigLoadFileInvalidJson() {
    QTemporaryFile file;
    file.open();
    file.write("invalid json {{{");
    file.close();

    sc::Config& config = sc::Config::instance();
    auto result = config.loadFile(file.fileName());
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), sc::ErrorCode::InternalError);
}

void TestConfiguration::testConfigLoadFromDirectory() {
    QTemporaryDir dir;
    QDir configDir(dir.path());

    QFile f1(configDir.filePath("config1.json"));
    f1.open(QIODevice::WriteOnly);
    f1.write(R"({"testcfg.dirname.key1":"value1"})");
    f1.close();

    QFile f2(configDir.filePath("config2.json"));
    f2.open(QIODevice::WriteOnly);
    f2.write(R"({"testcfg.dirname.key2":42})");
    f2.close();

    sc::Config& config = sc::Config::instance();
    auto result = config.loadFromDirectory(dir.path());
    QVERIFY(result.isOk());
    QCOMPARE(config.getString("testcfg.dirname.key1", ""), QString("value1"));
    QCOMPARE(config.getInt("testcfg.dirname.key2", 0), 42);
}

void TestConfiguration::testConfigLoadFromDirectoryNonExistent() {
    sc::Config& config = sc::Config::instance();
    auto result = config.loadFromDirectory("/nonexistent/directory/path");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), sc::ErrorCode::NotFound);
}

void TestConfiguration::testConfigLoadEnvironment() {
    QTemporaryDir dir;
    QDir configDir(dir.path());

    // Create a "dev" subdirectory with a JSON file
    QDir devDir(configDir.filePath("dev"));
    devDir.mkpath(".");

    QFile f(devDir.filePath("env.json"));
    f.open(QIODevice::WriteOnly);
    f.write(R"({"testcfg.env.level":"development"})");
    f.close();

    sc::Config& config = sc::Config::instance();
    // Must load directory first to set m_configDir
    config.loadFromDirectory(dir.path());
    auto result = config.loadEnvironment("dev");
    QVERIFY(result.isOk());
    QCOMPARE(config.getString("testcfg.env.level", ""), QString("development"));
}

void TestConfiguration::testConfigLoadEnvironmentNonExistent() {
    QTemporaryDir dir;

    sc::Config& config = sc::Config::instance();
    config.loadFromDirectory(dir.path());

    auto result = config.loadEnvironment("nonexistent_env");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), sc::ErrorCode::NotFound);
}

void TestConfiguration::testConfigSaveToFile() {
    sc::Config& config = sc::Config::instance();
    config.setString("testcfg.savetofile.key", "value");

    QTemporaryFile file;
    file.open();
    file.close();

    auto result = config.saveToFile(file.fileName());
    QVERIFY(result.isOk());

    // The saved file should contain valid JSON
    QFile f(file.fileName());
    f.open(QIODevice::ReadOnly);
    QByteArray content = f.readAll();
    f.close();

    QJsonDocument doc = QJsonDocument::fromJson(content);
    QVERIFY(doc.isObject());
}

void TestConfiguration::testConfigSaveAll() {
    sc::Config& config = sc::Config::instance();

    QTemporaryDir dir;
    config.loadFromDirectory(dir.path());

    auto result = config.saveAll();
    QVERIFY(result.isOk());
}

void TestConfiguration::testConfigHotReload() {
    sc::Config& config = sc::Config::instance();

    config.setHotReloadEnabled(true);
    QVERIFY(config.isHotReloadEnabled());

    config.setHotReloadEnabled(false);
    QVERIFY(!config.isHotReloadEnabled());
}

void TestConfiguration::testConfigChangeCallback() {
    sc::Config& config = sc::Config::instance();

    // Use shared_ptr so the callback captures remain valid even after the
    // callback outlives this test method (it persists in the singleton).
    auto called = std::make_shared<bool>(false);
    auto calledKey = std::make_shared<QString>();

    config.addChangeCallback([called, calledKey](const QString& key) {
        *called = true;
        *calledKey = key;
    });

    config.setString("testcfg.callback.key", "value");

    QVERIFY(*called);
    QCOMPARE(*calledKey, QString("testcfg.callback.key"));
}

// =========================================================================
// JsonConfiguration tests
// =========================================================================

void TestConfiguration::testJsonLoadFromFile() {
    QTemporaryFile file;
    file.open();
    file.write(R"({"host":"localhost","port":8080,"debug":true,"rate":3.14})");
    file.close();

    sc::JsonConfiguration config;
    auto result = config.load(file.fileName());
    QVERIFY(result.isOk());
    QCOMPARE(config.getString("host"), QString("localhost"));
    QCOMPARE(config.getInt("port"), 8080);
    QVERIFY(config.getBool("debug"));
    QCOMPARE(config.getDouble("rate"), 3.14);
}

void TestConfiguration::testJsonLoadNonExistent() {
    sc::JsonConfiguration config;
    auto result = config.load("/nonexistent/path/to/file.json");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), sc::ErrorCode::NotFound);
}

void TestConfiguration::testJsonLoadInvalidJson() {
    QTemporaryFile file;
    file.open();
    file.write("not valid json {{{");
    file.close();

    sc::JsonConfiguration config;
    auto result = config.load(file.fileName());
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), sc::ErrorCode::ParseError);
}

void TestConfiguration::testJsonSave() {
    sc::JsonConfiguration config;
    config.setString("host", "localhost");
    config.setInt("port", 5432);
    config.setBool("debug", true);
    config.setDouble("rate", 2.5);

    QTemporaryFile file;
    file.open();
    file.close();

    auto result = config.save(file.fileName());
    QVERIFY(result.isOk());

    // Reload and verify the saved data
    sc::JsonConfiguration config2;
    QVERIFY(config2.load(file.fileName()).isOk());
    QCOMPARE(config2.getString("host"), QString("localhost"));
    QCOMPARE(config2.getInt("port"), 5432);
    QVERIFY(config2.getBool("debug"));
    QCOMPARE(config2.getDouble("rate"), 2.5);
}

void TestConfiguration::testJsonGetValues() {
    QTemporaryFile file;
    file.open();
    file.write(R"({"strval":"text","intval":100,"dblval":1.5,"boolval":true})");
    file.close();

    sc::JsonConfiguration config;
    config.load(file.fileName());

    QCOMPARE(config.getString("strval"), QString("text"));
    QCOMPARE(config.getInt("intval"), 100);
    QCOMPARE(config.getDouble("dblval"), 1.5);
    QVERIFY(config.getBool("boolval"));
}

void TestConfiguration::testJsonSetValues() {
    sc::JsonConfiguration config;
    config.setString("skey", "sval");
    config.setInt("ikey", 999);
    config.setDouble("dkey", 9.9);
    config.setBool("bkey", true);

    QCOMPARE(config.getString("skey"), QString("sval"));
    QCOMPARE(config.getInt("ikey"), 999);
    QCOMPARE(config.getDouble("dkey"), 9.9);
    QVERIFY(config.getBool("bkey"));
}

void TestConfiguration::testJsonContains() {
    sc::JsonConfiguration config;
    config.setString("key", "value");
    QVERIFY(config.contains("key"));
    QVERIFY(!config.contains("missing"));
}

void TestConfiguration::testJsonRemove() {
    sc::JsonConfiguration config;
    config.setString("key", "value");
    QVERIFY(config.contains("key"));
    config.remove("key");
    QVERIFY(!config.contains("key"));
}

void TestConfiguration::testJsonDefaults() {
    sc::JsonConfiguration config;
    QCOMPARE(config.getString("missing", "default"), QString("default"));
    QCOMPARE(config.getInt("missing", 100), 100);
    QCOMPARE(config.getDouble("missing", 1.5), 1.5);
    QVERIFY(config.getBool("missing", true));
}

void TestConfiguration::testJsonDataAccessor() {
    sc::JsonConfiguration config;
    config.setString("key1", "val1");
    config.setInt("key2", 42);

    const QJsonObject& data = config.data();
    QVERIFY(data.contains("key1"));
    QVERIFY(data.contains("key2"));
    QCOMPARE(data.value("key1").toString(), QString("val1"));
}

// =========================================================================
// IniConfiguration tests
// =========================================================================

void TestConfiguration::testIniLoadFromFile() {
    QTemporaryFile file;
    file.open();
    file.write("[database]\nhost=localhost\nport=5432\n");
    file.close();

    sc::IniConfiguration config;
    auto result = config.load(file.fileName());
    QVERIFY(result.isOk());
    QCOMPARE(config.getString("database/host", ""), QString("localhost"));
    QCOMPARE(config.getInt("database/port", 0), 5432);
}

void TestConfiguration::testIniSave() {
    QTemporaryFile file;
    file.open();
    file.write("[server]\nhost=oldhost\n");
    file.close();

    sc::IniConfiguration config;
    QVERIFY(config.load(file.fileName()).isOk());

    config.setString("server/host", "newhost");
    QVERIFY(config.save(file.fileName()).isOk());

    // Reload and verify the saved value
    sc::IniConfiguration config2;
    QVERIFY(config2.load(file.fileName()).isOk());
    QCOMPARE(config2.getString("server/host", ""), QString("newhost"));
}

void TestConfiguration::testIniGetSetValues() {
    QTemporaryFile file;
    file.open();
    file.close();

    sc::IniConfiguration config;
    config.load(file.fileName());

    config.setString("sec/strkey", "strval");
    config.setInt("sec/intkey", 777);
    config.setDouble("sec/dblkey", 4.5);
    config.setBool("sec/boolkey", true);

    QCOMPARE(config.getString("sec/strkey", ""), QString("strval"));
    QCOMPARE(config.getInt("sec/intkey", 0), 777);
    QCOMPARE(config.getDouble("sec/dblkey", 0.0), 4.5);
    QVERIFY(config.getBool("sec/boolkey", false));
}

void TestConfiguration::testIniContains() {
    QTemporaryFile file;
    file.open();
    file.close();

    sc::IniConfiguration config;
    config.load(file.fileName());
    config.setString("sec/key", "value");
    QVERIFY(config.contains("sec/key"));
    QVERIFY(!config.contains("sec/missing"));
}

void TestConfiguration::testIniRemove() {
    QTemporaryFile file;
    file.open();
    file.close();

    sc::IniConfiguration config;
    config.load(file.fileName());
    config.setString("sec/key", "value");
    QVERIFY(config.contains("sec/key"));
    config.remove("sec/key");
    QVERIFY(!config.contains("sec/key"));
}

// =========================================================================
// ConfigSchema tests
// =========================================================================

void TestConfiguration::testSchemaAddField() {
    sc::ConfigSchema schema;

    sc::ConfigField field;
    field.name = "host";
    field.type = sc::ConfigType::String;
    field.defaultValue = "localhost";
    field.required = true;
    field.description = "Server host";
    schema.addField(field);

    const auto& fields = schema.fields();
    QCOMPARE(static_cast<int>(fields.size()), 1);
    QCOMPARE(fields[0].name, QString("host"));
    QCOMPARE(fields[0].type, sc::ConfigType::String);
    QVERIFY(fields[0].required);
}

void TestConfiguration::testSchemaValidateRequired() {
    sc::ConfigSchema schema;

    sc::ConfigField field;
    field.name = "host";
    field.type = sc::ConfigType::String;
    field.required = true;
    schema.addField(field);

    // Missing required field -> invalid
    QVariantMap emptyConfig;
    QString errorMsg;
    QVERIFY(!schema.validate(emptyConfig, &errorMsg));
    QVERIFY(errorMsg.contains("Missing required field"));

    // With required field -> valid
    QVariantMap validConfig;
    validConfig["host"] = "localhost";
    QVERIFY(schema.validate(validConfig));
}

void TestConfiguration::testSchemaValidateTypes() {
    sc::ConfigSchema schema;

    sc::ConfigField strField;
    strField.name = "host";
    strField.type = sc::ConfigType::String;
    schema.addField(strField);

    sc::ConfigField intField;
    intField.name = "port";
    intField.type = sc::ConfigType::Int;
    schema.addField(intField);

    sc::ConfigField dblField;
    dblField.name = "rate";
    dblField.type = sc::ConfigType::Double;
    schema.addField(dblField);

    sc::ConfigField boolField;
    boolField.name = "enabled";
    boolField.type = sc::ConfigType::Bool;
    schema.addField(boolField);

    // Valid config with correct types
    QVariantMap validConfig;
    validConfig["host"] = "localhost";
    validConfig["port"] = 8080;
    validConfig["rate"] = 3.14;
    validConfig["enabled"] = true;

    QString errorMsg;
    QVERIFY(schema.validate(validConfig, &errorMsg));

    // Invalid: QVariantMap (object) where Int expected
    QVariantMap invalidConfig;
    invalidConfig["port"] = QVariantMap();
    QVERIFY(!schema.validate(invalidConfig, &errorMsg));
    QVERIFY(errorMsg.contains("must be an integer"));
}

void TestConfiguration::testSchemaValidateCustomValidator() {
    sc::ConfigSchema schema;

    sc::ConfigField field;
    field.name = "port";
    field.type = sc::ConfigType::Int;
    field.required = true;
    field.validator = [](const QVariant& v) -> bool {
        int port = v.toInt();
        return port > 0 && port <= 65535;
    };
    schema.addField(field);

    // Valid port
    QVariantMap validConfig;
    validConfig["port"] = 8080;
    QVERIFY(schema.validate(validConfig));

    // Invalid port (out of range, custom validator fails)
    QVariantMap invalidConfig;
    invalidConfig["port"] = 70000;
    QString errorMsg;
    QVERIFY(!schema.validate(invalidConfig, &errorMsg));
    QVERIFY(errorMsg.contains("validation failed"));
}

void TestConfiguration::testSchemaApplyDefaults() {
    sc::ConfigSchema schema;

    sc::ConfigField hostField;
    hostField.name = "host";
    hostField.type = sc::ConfigType::String;
    hostField.defaultValue = "localhost";
    schema.addField(hostField);

    sc::ConfigField portField;
    portField.name = "port";
    portField.type = sc::ConfigType::Int;
    portField.defaultValue = 8080;
    schema.addField(portField);

    QVariantMap config;
    config["host"] = "example.com";  // override default

    QVariantMap result = schema.applyDefaults(config);
    QCOMPARE(result.value("host").toString(), QString("example.com"));
    QVERIFY(result.contains("port"));
    QCOMPARE(result.value("port").toInt(), 8080);
}

void TestConfiguration::testSchemaValidateOptionalField() {
    sc::ConfigSchema schema;

    sc::ConfigField requiredField;
    requiredField.name = "host";
    requiredField.type = sc::ConfigType::String;
    requiredField.required = true;
    schema.addField(requiredField);

    sc::ConfigField optionalField;
    optionalField.name = "port";
    optionalField.type = sc::ConfigType::Int;
    optionalField.required = false;
    optionalField.defaultValue = 8080;
    schema.addField(optionalField);

    // Only required field present -> valid
    QVariantMap config;
    config["host"] = "localhost";
    QVERIFY(schema.validate(config));

    // Both fields present -> valid
    config["port"] = 9090;
    QVERIFY(schema.validate(config));
}

void TestConfiguration::testEmptyConfigSchema() {
    sc::ConfigSchema schema;
    QVariantMap config;
    QVERIFY(schema.validate(config));

    // applyDefaults on empty schema returns the same config
    QVariantMap result = schema.applyDefaults(config);
    QVERIFY(result.isEmpty());
}

// =========================================================================
// Edge cases
// =========================================================================

void TestConfiguration::testMissingKeysReturnDefaults() {
    sc::Config& config = sc::Config::instance();
    QCOMPARE(config.getString("testcfg.edge.missing.str", "strdefault"), QString("strdefault"));
    QCOMPARE(config.getInt("testcfg.edge.missing.int", -1), -1);
    QCOMPARE(config.getDouble("testcfg.edge.missing.dbl", -1.0), -1.0);
    QVERIFY(!config.getBool("testcfg.edge.missing.bool", false));
}

void TestConfiguration::testInvalidJsonType() {
    // Test that type mismatches return defaults for JsonConfiguration
    sc::JsonConfiguration config;
    config.setInt("numkey", 42);
    config.setBool("boolkey", true);

    // getInt on a bool key returns default (bool is not Double in QJsonValue)
    QCOMPARE(config.getInt("boolkey", -1), -1);
    // getBool on an int key returns default (int is stored as Double, not Bool)
    QVERIFY(!config.getBool("numkey", false));
    // getString on an int key returns default
    QCOMPARE(config.getString("numkey", "default"), QString("default"));
}

void TestConfiguration::testEnvVarOverrideEdgeCases() {
    sc::Config& config = sc::Config::instance();
    config.setEnvPrefix("SOUL_");

    // Bool env var variants: "true", "1", "yes" -> true
    qputenv("SOUL_TESTCFG_ENVEDGE_BOOL", "yes");
    QVERIFY(config.getBool("testcfg.envedge.bool", false));
    qunsetenv("SOUL_TESTCFG_ENVEDGE_BOOL");

    qputenv("SOUL_TESTCFG_ENVEDGE_BOOL", "1");
    QVERIFY(config.getBool("testcfg.envedge.bool", false));
    qunsetenv("SOUL_TESTCFG_ENVEDGE_BOOL");

    qputenv("SOUL_TESTCFG_ENVEDGE_BOOL", "true");
    QVERIFY(config.getBool("testcfg.envedge.bool", false));
    qunsetenv("SOUL_TESTCFG_ENVEDGE_BOOL");

    // Bool env var: "no", "false", "0" -> false
    qputenv("SOUL_TESTCFG_ENVEDGE_BOOL", "no");
    QVERIFY(!config.getBool("testcfg.envedge.bool", true));
    qunsetenv("SOUL_TESTCFG_ENVEDGE_BOOL");

    // Double env var override
    qputenv("SOUL_TESTCFG_ENVEDGE_DBL", "2.71828");
    QCOMPARE(config.getDouble("testcfg.envedge.dbl", 0.0), 2.71828);
    qunsetenv("SOUL_TESTCFG_ENVEDGE_DBL");

    // String env var override
    qputenv("SOUL_TESTCFG_ENVEDGE_STR", "fromenv");
    QCOMPARE(config.getString("testcfg.envedge.str", ""), QString("fromenv"));
    qunsetenv("SOUL_TESTCFG_ENVEDGE_STR");
}

QTEST_MAIN(TestConfiguration)
#include "test_configuration.moc"