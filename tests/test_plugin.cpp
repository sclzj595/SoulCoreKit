#include <QTest>

#include "soul/plugin/iplugin.h"
#include "soul/plugin/module.h"
#include "soul/plugin/plugin_manager.h"

class TestPlugin : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testPluginMetadataStructure();
    void testPluginAbiVersion();
    void testPluginManagerSingleton();
    void testLoadNonExistentPlugin();
    void testPluginCount();
    void testGetPluginIdsEmpty();
    void testIsPluginLoaded();
    void testIsPluginInitialized();
    void testInitializePluginLocked();
    void testShutdownPluginLocked();
    void testInitializeAllPlugins();
    void testShutdownAllPlugins();
};

void TestPlugin::initTestCase()
{
    sc::plugin::Module::initialize();
}

void TestPlugin::cleanupTestCase()
{
    sc::plugin::Module::shutdown();
}

void TestPlugin::testPluginMetadataStructure()
{
    sc::plugin::PluginMetadata metadata;
    metadata.id = "com.soulcore.plugin.test";
    metadata.name = "Test Plugin";
    metadata.version = "1.0.0";
    metadata.description = "Test plugin for unit testing";
    metadata.author = "Test Author";
    metadata.dependencies = "";
    metadata.abiVersion = PLUGIN_ABI_VERSION;
    metadata.apiVersion = PLUGIN_API_VERSION;

    QCOMPARE(metadata.id, "com.soulcore.plugin.test");
    QCOMPARE(metadata.name, "Test Plugin");
    QCOMPARE(metadata.version, "1.0.0");
    QCOMPARE(metadata.abiVersion, PLUGIN_ABI_VERSION);
    QCOMPARE(metadata.apiVersion, PLUGIN_API_VERSION);
}

void TestPlugin::testPluginAbiVersion()
{
    QVERIFY(PLUGIN_ABI_VERSION > 0);
    QVERIFY(PLUGIN_API_VERSION > 0);
}

void TestPlugin::testPluginManagerSingleton()
{
    auto& pm1 = sc::plugin::PluginManager::instance();
    auto& pm2 = sc::plugin::PluginManager::instance();
    QVERIFY(&pm1 == &pm2);
}

void TestPlugin::testLoadNonExistentPlugin()
{
    auto& pm = sc::plugin::PluginManager::instance();
    bool result = pm.loadPlugin("/nonexistent/path/plugin.dll");
    QVERIFY(!result);
    QCOMPARE(pm.pluginCount(), 0);
}

void TestPlugin::testPluginCount()
{
    auto& pm = sc::plugin::PluginManager::instance();
    int initialCount = pm.pluginCount();
    QVERIFY(initialCount >= 0);
}

void TestPlugin::testGetPluginIdsEmpty()
{
    auto& pm = sc::plugin::PluginManager::instance();
    auto ids = pm.getPluginIds();
    QVERIFY(ids.size() == static_cast<size_t>(pm.pluginCount()));
}

void TestPlugin::testIsPluginLoaded()
{
    auto& pm = sc::plugin::PluginManager::instance();
    QVERIFY(!pm.isPluginLoaded("nonexistent.plugin.id"));
}

void TestPlugin::testIsPluginInitialized()
{
    auto& pm = sc::plugin::PluginManager::instance();
    QVERIFY(!pm.isPluginInitialized("nonexistent.plugin.id"));
}

void TestPlugin::testInitializePluginLocked()
{
    auto& pm = sc::plugin::PluginManager::instance();
    bool result = pm.initializePlugin("nonexistent.plugin.id");
    QVERIFY(!result);
}

void TestPlugin::testShutdownPluginLocked()
{
    auto& pm = sc::plugin::PluginManager::instance();
    bool result = pm.shutdownPlugin("nonexistent.plugin.id");
    QVERIFY(!result);
}

void TestPlugin::testInitializeAllPlugins()
{
    auto& pm = sc::plugin::PluginManager::instance();
    pm.initializeAllPlugins();
    QVERIFY(true);
}

void TestPlugin::testShutdownAllPlugins()
{
    auto& pm = sc::plugin::PluginManager::instance();
    pm.shutdownAllPlugins();
    QVERIFY(true);
}

QTEST_APPLESS_MAIN(TestPlugin)

#include "test_plugin.moc"
