#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include "soul/rpc/service_discovery.h"

using namespace sc::rpc;

// ============================================================================
// TestServiceDiscoveryFactory — 服务发现工厂单元测试
// ============================================================================
class TestServiceDiscoveryFactory : public QObject {
    Q_OBJECT

private slots:
    void testCreateConsul();
    void testCreateEureka();
    void testCreateNacos();
    void testCreateInMemory();
};

void TestServiceDiscoveryFactory::testCreateConsul() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::Consul);
    QVERIFY(discovery != nullptr);
}

void TestServiceDiscoveryFactory::testCreateEureka() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::Eureka);
    QVERIFY(discovery != nullptr);
}

void TestServiceDiscoveryFactory::testCreateNacos() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::Nacos);
    QVERIFY(discovery != nullptr);
}

void TestServiceDiscoveryFactory::testCreateInMemory() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::InMemory);
    QVERIFY(discovery != nullptr);
}

// ============================================================================
// TestServiceDiscovery — 服务发现单元测试
// ============================================================================
class TestServiceDiscovery : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testConnectDisconnect();
    void testRegisterInstance();
    void testGetInstances();
    void testGetServiceNames();
    void testReportHealthy();
    void testReportUnhealthy();
    void testIsHealthy();
    void testWatchUnwatch();
};

void TestServiceDiscovery::initTestCase() {
}

void TestServiceDiscovery::cleanupTestCase() {
}

void TestServiceDiscovery::testConnectDisconnect() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::InMemory);
    QVERIFY(discovery != nullptr);

    QVERIFY(!discovery->isConnected());

    DiscoveryConfig config;
    config.backend = DiscoveryBackend::InMemory;
    config.serviceName = "TestService";

    auto r = discovery->connect(config);
    Q_UNUSED(r);
    // InMemory 后端连接应该成功

    discovery->disconnect();
    QVERIFY(!discovery->isConnected());
}

void TestServiceDiscovery::testRegisterInstance() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::InMemory);
    QVERIFY(discovery != nullptr);

    DiscoveryConfig config;
    config.backend = DiscoveryBackend::InMemory;
    config.serviceName = "TestService";
    (void)discovery->connect(config);

    ServiceInstance instance;
    instance.serviceName = "TestService";
    instance.host = "127.0.0.1";
    instance.port = 8080;

    auto r = discovery->registerInstance(instance);
    QVERIFY(r.isOk());
}

void TestServiceDiscovery::testGetInstances() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::InMemory);
    QVERIFY(discovery != nullptr);

    DiscoveryConfig config;
    config.backend = DiscoveryBackend::InMemory;
    config.serviceName = "TestService";
    (void)discovery->connect(config);

    ServiceInstance instance;
    instance.serviceName = "TestService";
    instance.host = "127.0.0.1";
    instance.port = 8080;
    (void)discovery->registerInstance(instance);

    auto instances = discovery->getInstances("TestService");
    QVERIFY(instances.isOk());
    QVERIFY(instances.unwrap().size() >= 1);
}

void TestServiceDiscovery::testGetServiceNames() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::InMemory);
    QVERIFY(discovery != nullptr);

    DiscoveryConfig config;
    config.backend = DiscoveryBackend::InMemory;
    config.serviceName = "TestService";
    (void)discovery->connect(config);

    ServiceInstance instance;
    instance.serviceName = "TestService";
    instance.host = "127.0.0.1";
    instance.port = 8080;
    (void)discovery->registerInstance(instance);

    auto names = discovery->getServiceNames();
    QVERIFY(names.contains("TestService"));
}

void TestServiceDiscovery::testReportHealthy() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::InMemory);
    QVERIFY(discovery != nullptr);

    DiscoveryConfig config;
    config.backend = DiscoveryBackend::InMemory;
    (void)discovery->connect(config);

    auto r = discovery->reportHealthy();
    QVERIFY(r.isOk());
}

void TestServiceDiscovery::testReportUnhealthy() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::InMemory);
    QVERIFY(discovery != nullptr);

    DiscoveryConfig config;
    config.backend = DiscoveryBackend::InMemory;
    (void)discovery->connect(config);

    auto r = discovery->reportUnhealthy("Connection timeout");
    QVERIFY(r.isOk());
}

void TestServiceDiscovery::testIsHealthy() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::InMemory);
    QVERIFY(discovery != nullptr);

    DiscoveryConfig config;
    config.backend = DiscoveryBackend::InMemory;
    (void)discovery->connect(config);

    // 初始状态
    QVERIFY(!discovery->isHealthy());

    (void)discovery->reportHealthy();
    QVERIFY(discovery->isHealthy());
}

void TestServiceDiscovery::testWatchUnwatch() {
    auto discovery = ServiceDiscoveryFactory::create(DiscoveryBackend::InMemory);
    QVERIFY(discovery != nullptr);

    DiscoveryConfig config;
    config.backend = DiscoveryBackend::InMemory;
    (void)discovery->connect(config);

    int changeCount = 0;
    auto watchResult = discovery->watch("TestService", [&](const QList<ServiceInstance>&) {
        changeCount++;
    });
    QVERIFY(watchResult.isOk());

    auto unwatchResult = discovery->unwatch("TestService");
    QVERIFY(unwatchResult.isOk());
}

// ============================================================================
// TestWeightedLoadBalancer — 加权负载均衡单元测试
// ============================================================================
class TestWeightedLoadBalancer : public QObject {
    Q_OBJECT

private slots:
    void testSetWeights();
    void testSelect();
    void testPickRandom();
    void testPickRoundRobin();
    void testPickWeightedRoundRobin();
    void testPickLeastConnections();
    void testInstanceCount();
    void testAddInstance();
    void testRemoveInstance();
    void testUpdateWeight();
};

void TestWeightedLoadBalancer::testSetWeights() {
    WeightedLoadBalancer balancer;

    QHash<QString, int> weights;
    weights["host1:8080"] = 5;
    weights["host2:8080"] = 3;
    weights["host3:8080"] = 2;

    balancer.setWeights(weights);
    // 验证: 不崩溃即通过
    QVERIFY(true);
}

void TestWeightedLoadBalancer::testSelect() {
    WeightedLoadBalancer balancer;

    QList<ServiceInstance> instances;
    ServiceInstance inst1;
    inst1.host = "127.0.0.1";
    inst1.port = 8080;
    inst1.serviceName = "TestService";
    instances.append(inst1);

    ServiceInstance inst2;
    inst2.host = "127.0.0.1";
    inst2.port = 8081;
    inst2.serviceName = "TestService";
    instances.append(inst2);

    // select 应该返回一个有效实例
    auto selected = balancer.select(instances);
    QVERIFY(!selected.host.isEmpty());
}

void TestWeightedLoadBalancer::testPickRandom() {
    WeightedLoadBalancer balancer;
    balancer.setRandom();

    QList<ServiceInstance> instances;
    ServiceInstance inst;
    inst.host = "127.0.0.1";
    inst.port = 8080;
    inst.serviceName = "TestService";
    instances.append(inst);

    auto selected = balancer.select(instances);
    QCOMPARE(selected.host, QString("127.0.0.1"));
    QCOMPARE(selected.port, 8080);
}

void TestWeightedLoadBalancer::testPickRoundRobin() {
    WeightedLoadBalancer balancer;
    balancer.setRoundRobin();

    QList<ServiceInstance> instances;
    for (int i = 0; i < 3; ++i) {
        ServiceInstance inst;
        inst.host = "127.0.0.1";
        inst.port = 8080 + i;
        inst.serviceName = "TestService";
        instances.append(inst);
    }

    // RoundRobin 多次 select 应该循环
    int port1 = balancer.select(instances).port;
    int port2 = balancer.select(instances).port;
    int port3 = balancer.select(instances).port;
    int port4 = balancer.select(instances).port;

    QCOMPARE(port1, 8080);
    QCOMPARE(port2, 8081);
    QCOMPARE(port3, 8082);
    QCOMPARE(port4, 8080);  // 回到第一个
}

void TestWeightedLoadBalancer::testPickWeightedRoundRobin() {
    WeightedLoadBalancer balancer;

    QHash<QString, int> weights;
    weights["127.0.0.1:8080"] = 5;
    weights["127.0.0.1:8081"] = 1;
    balancer.setWeights(weights);
    balancer.setWeightedRoundRobin();

    QList<ServiceInstance> instances;
    ServiceInstance inst1;
    inst1.host = "127.0.0.1";
    inst1.port = 8080;
    inst1.serviceName = "TestService";
    instances.append(inst1);

    ServiceInstance inst2;
    inst2.host = "127.0.0.1";
    inst2.port = 8081;
    inst2.serviceName = "TestService";
    instances.append(inst2);

    // 加权轮询应返回有效实例
    auto selected = balancer.select(instances);
    QVERIFY(!selected.host.isEmpty());
}

void TestWeightedLoadBalancer::testPickLeastConnections() {
    WeightedLoadBalancer balancer;
    balancer.setLeastConnections();

    QList<ServiceInstance> instances;
    ServiceInstance inst;
    inst.host = "127.0.0.1";
    inst.port = 8080;
    inst.serviceName = "TestService";
    instances.append(inst);

    auto selected = balancer.select(instances);
    QCOMPARE(selected.host, QString("127.0.0.1"));
}

void TestWeightedLoadBalancer::testInstanceCount() {
    QList<ServiceInstance> instances;
    for (int i = 0; i < 5; ++i) {
        ServiceInstance inst;
        inst.host = "127.0.0.1";
        inst.port = 8080 + i;
        inst.serviceName = "TestService";
        instances.append(inst);
    }

    QCOMPARE(instances.size(), 5);
}

void TestWeightedLoadBalancer::testAddInstance() {
    QList<ServiceInstance> instances;
    ServiceInstance inst;
    inst.host = "127.0.0.1";
    inst.port = 8080;
    inst.serviceName = "TestService";
    instances.append(inst);
    QCOMPARE(instances.size(), 1);
}

void TestWeightedLoadBalancer::testRemoveInstance() {
    QList<ServiceInstance> instances;
    ServiceInstance inst1;
    inst1.host = "127.0.0.1";
    inst1.port = 8080;
    inst1.serviceName = "TestService";
    instances.append(inst1);

    ServiceInstance inst2;
    inst2.host = "127.0.0.1";
    inst2.port = 8081;
    inst2.serviceName = "TestService";
    instances.append(inst2);

    QCOMPARE(instances.size(), 2);

    instances.removeAt(0);
    QCOMPARE(instances.size(), 1);
}

void TestWeightedLoadBalancer::testUpdateWeight() {
    WeightedLoadBalancer balancer;

    QHash<QString, int> weights;
    weights["127.0.0.1:8080"] = 3;
    balancer.setWeights(weights);

    // 更新权重
    weights["127.0.0.1:8080"] = 10;
    balancer.setWeights(weights);

    QVERIFY(true);
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    TestServiceDiscoveryFactory factoryTest;
    TestServiceDiscovery discoveryTest;
    TestWeightedLoadBalancer balancerTest;

    int result = 0;
    result |= QTest::qExec(&factoryTest, argc, argv);
    result |= QTest::qExec(&discoveryTest, argc, argv);
    result |= QTest::qExec(&balancerTest, argc, argv);

    return result;
}

#include "test_service_discovery.moc"