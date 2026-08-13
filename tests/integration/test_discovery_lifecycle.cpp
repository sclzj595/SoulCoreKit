// ============================================================================
// test_discovery_lifecycle.cpp — ServiceDiscovery 生命周期测试 [v2.9.3]
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include "soul/rpc/service_discovery.h"

using namespace sc::rpc;

class TestDiscoveryLifecycle : public QObject {
    Q_OBJECT

private slots:
    // 1. 完整生命周期
    void testFullLifecycle() {
        InMemoryServiceDiscovery discovery;
        DiscoveryConfig config;
        config.serviceName = "test-service";
        config.host = "127.0.0.1";
        config.port = 8080;

        QVERIFY(discovery.connect(config).isOk());
        QVERIFY(discovery.isConnected());

        ServiceInstance inst{"test-service", "inst-1", "127.0.0.1", 8080, 0, ServiceInstanceStatus::Unknown, {}, {}};
        QVERIFY(discovery.registerInstance(inst).isOk());

        auto instances = discovery.getInstances("test-service");
        QVERIFY(instances.isOk());
        QCOMPARE(instances.unwrap().size(), 1);

        QVERIFY(discovery.unregisterInstance("test-service", "127.0.0.1", 8080).isOk());
        discovery.disconnect();
        QVERIFY(!discovery.isConnected());
    }

    // 2. disconnect 幂等
    void testDisconnectIdempotent() {
        InMemoryServiceDiscovery discovery;
        DiscoveryConfig config;

        discovery.connect(config);
        discovery.disconnect();
        discovery.disconnect();  // 第二次 — 不崩溃
        QVERIFY(!discovery.isConnected());
    }

    // 3. connect → disconnect → connect
    void testReconnect() {
        InMemoryServiceDiscovery discovery;
        DiscoveryConfig config;

        QVERIFY(discovery.connect(config).isOk());
        discovery.disconnect();

        QVERIFY(discovery.connect(config).isOk());
        QVERIFY(discovery.isConnected());
    }

    // 4. unregister 未注册实例
    void testUnregisterNotFound() {
        InMemoryServiceDiscovery discovery;
        discovery.connect(DiscoveryConfig{});

        auto result = discovery.unregisterInstance("nonexistent", "1.2.3.4", 9999);
        QVERIFY(result.isErr());
    }

    // 5. getInstances 空服务
    void testGetInstancesEmpty() {
        InMemoryServiceDiscovery discovery;
        discovery.connect(DiscoveryConfig{});

        auto result = discovery.getInstances("nonexistent");
        QVERIFY(result.isOk());
        QVERIFY(result.unwrap().isEmpty());
    }

    // 6. reportHealthy / reportUnhealthy
    void testHealthReporting() {
        InMemoryServiceDiscovery discovery;
        discovery.connect(DiscoveryConfig{});

        QVERIFY(discovery.reportHealthy().isOk());
        QVERIFY(discovery.isHealthy());

        QVERIFY(discovery.reportUnhealthy("test").isOk());
        QVERIFY(!discovery.isHealthy());
    }

    // 7. getInstance by instanceId [v2.9.3]
    void testGetInstanceById() {
        InMemoryServiceDiscovery discovery;
        discovery.connect(DiscoveryConfig{});

        ServiceInstance inst{"svc", "my-id-123", "10.0.0.1", 9090, 0, ServiceInstanceStatus::Unknown, {}, {}};
        inst.status = ServiceInstanceStatus::Up;
        discovery.registerInstance(inst);

        auto result = discovery.getInstance("svc", "my-id-123");
        QVERIFY(result.isOk());
        QVERIFY(result.unwrap().has_value());
        QCOMPARE(result.unwrap()->host, QString("10.0.0.1"));

        auto miss = discovery.getInstance("svc", "nonexistent");
        QVERIFY(miss.isOk());
        QVERIFY(!miss.unwrap().has_value());
    }

    // 8. getAllServices [v2.9.3]
    void testGetAllServices() {
        InMemoryServiceDiscovery discovery;
        discovery.connect(DiscoveryConfig{});

        discovery.registerInstance({"svc-a", "a1", "h1", 1, 0, ServiceInstanceStatus::Unknown, {}, {}});
        discovery.registerInstance({"svc-b", "b1", "h2", 2, 0, ServiceInstanceStatus::Unknown, {}, {}});

        auto result = discovery.getAllServices();
        QVERIFY(result.isOk());
        auto names = result.unwrap();
        QVERIFY(names.contains("svc-a"));
        QVERIFY(names.contains("svc-b"));
    }

    // 9. ServiceInstance::uniqueKey
    void testInstanceUniqueKey() {
        ServiceInstance a{"svc", "id-1", "h1", 1, 0, ServiceInstanceStatus::Unknown, {}, {}};
        ServiceInstance b{"svc", "id-1", "h1", 1, 0, ServiceInstanceStatus::Unknown, {}, {}};
        QCOMPARE(a.uniqueKey(), b.uniqueKey());

        ServiceInstance c{"svc", "", "h1", 1, 0, ServiceInstanceStatus::Unknown, {}, {}};
        QVERIFY(!c.uniqueKey().isEmpty());
    }
};

QTEST_MAIN(TestDiscoveryLifecycle)
#include "test_discovery_lifecycle.moc"
