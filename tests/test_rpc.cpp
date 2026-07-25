#include <QtTest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

#include "soul/rpc/iserializer.h"
#include "soul/rpc/irpc_transport.h"
#include "soul/rpc/service_dispatcher.h"
#include "soul/rpc/client_proxy.h"
#include "soul/rpc/service_registry.h"
#include "soul/rpc/http_transport.h"

class MockTransport : public sc::rpc::IRpcTransport {
public:
    void setResponse(const sc::rpc::RpcResponse& resp) {
        m_response = resp;
        m_error = sc::Error{sc::ErrorCode::Ok, ""};
    }
    void setError(const sc::Error& err) { m_error = err; }

    sc::Result<sc::rpc::RpcResponse> sendRequest(const sc::rpc::RpcRequest& request) override {
        m_lastRequest = request;
        if (m_error.code() != sc::ErrorCode::Ok) {
            return m_error;
        }
        return m_response;
    }
    void start() override { m_running = true; }
    void stop() override { m_running = false; }
    bool isRunning() const override { return m_running; }
    sc::rpc::RpcRequest lastRequest() const { return m_lastRequest; }

private:
    sc::rpc::RpcResponse m_response;
    sc::Error m_error{sc::ErrorCode::Ok, ""};
    sc::rpc::RpcRequest m_lastRequest;
    bool m_running = false;
};

class TestRpc : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testJsonSerializeDeserialize();
    void testJsonSerializeDeserialize_data();

    void testServiceRegisterAndDispatch();
    void testServiceUnregister();
    void testServiceNotFound();
    void testServiceList();

    void testClientProxyCall();
    void testClientProxyCallError();
    void testClientProxyCallParse();
    void testClientProxyCallParse_data();

    void testServiceRegistryRegister();
    void testServiceRegistryUnregister();
    void testServiceRegistryGet();
    void testServiceRegistryEmpty();

    void testLoadBalancerRoundRobin();
    void testLoadBalancerRandom();

    void testRpcValueSerialization();

private:
    std::shared_ptr<sc::rpc::JsonSerializer> m_serializer;
    sc::rpc::ServiceDispatcher m_dispatcher;
    std::shared_ptr<MockTransport> m_mockTransport;
    std::shared_ptr<sc::rpc::ClientProxy> m_client;
};

void TestRpc::initTestCase() {
    m_serializer = std::make_shared<sc::rpc::JsonSerializer>();
    m_mockTransport = std::make_shared<MockTransport>();
    m_client = std::make_shared<sc::rpc::ClientProxy>(m_mockTransport, m_serializer);
}

void TestRpc::cleanupTestCase() {
}

void TestRpc::testJsonSerializeDeserialize() {
    QTEST_SET_MAIN_SOURCE_PATH
    QJsonObject obj;
    obj["name"] = "test";
    obj["value"] = 42;
    obj["active"] = true;

    QByteArray data = m_serializer->serialize(obj);
    QVERIFY(!data.isEmpty());

    auto result = m_serializer->deserialize(data);
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap()["name"].toString(), QString("test"));
    QCOMPARE(result.unwrap()["value"].toInteger(), 42);
    QCOMPARE(result.unwrap()["active"].toBool(), true);
}

void TestRpc::testJsonSerializeDeserialize_data() {
    QTest::addColumn<QJsonObject>("input");

    QJsonObject obj1;
    obj1["string"] = "hello";
    obj1["int"] = 123;
    obj1["double"] = 3.14;
    obj1["bool"] = false;
    QTest::newRow("mixed types") << obj1;

    QJsonObject obj2;
    obj2["empty"] = QJsonObject();
    QTest::newRow("empty object") << obj2;
}

void TestRpc::testServiceRegisterAndDispatch() {
    bool called = false;
    m_dispatcher.registerService("TestService", [&called](const sc::rpc::RpcRequest& req) -> sc::rpc::RpcResponse {
        called = true;
        QJsonObject data;
        data["result"] = "hello";
        return sc::rpc::RpcResponse{true, data, "", req.requestId};
    });

    sc::rpc::RpcRequest req;
    req.serviceName = "TestService";
    req.methodName = "getGreeting";
    req.requestId = "req-1";

    auto resp = m_dispatcher.dispatch(req);
    QVERIFY(called);
    QVERIFY(resp.success);
    QCOMPARE(resp.data["result"].toString(), QString("hello"));
    QCOMPARE(resp.requestId, QString("req-1"));
}

void TestRpc::testServiceUnregister() {
    m_dispatcher.registerService("TempService", [](const sc::rpc::RpcRequest&) -> sc::rpc::RpcResponse {
        return {true, {}, "", ""};
    });
    m_dispatcher.unregisterService("TempService");

    sc::rpc::RpcRequest req;
    req.serviceName = "TempService";
    auto resp = m_dispatcher.dispatch(req);
    QVERIFY(!resp.success);
}

void TestRpc::testServiceNotFound() {
    sc::rpc::RpcRequest req;
    req.serviceName = "NonExistent";
    auto resp = m_dispatcher.dispatch(req);
    QVERIFY(!resp.success);
    QVERIFY(!resp.errorMessage.isEmpty());
}

void TestRpc::testServiceList() {
    m_dispatcher.registerService("SvcA", [](const sc::rpc::RpcRequest&) -> sc::rpc::RpcResponse {
        return {true, {}, "", ""};
    });
    m_dispatcher.registerService("SvcB", [](const sc::rpc::RpcRequest&) -> sc::rpc::RpcResponse {
        return {true, {}, "", ""};
    });

    auto services = m_dispatcher.registeredServices();
    QVERIFY(services.contains("SvcA"));
    QVERIFY(services.contains("SvcB"));
}

void TestRpc::testClientProxyCall() {
    sc::rpc::RpcResponse resp;
    resp.success = true;
    resp.data["result"] = "world";
    m_mockTransport->setResponse(resp);

    auto result = m_client->call("Svc", "method");
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap()["result"].toString(), QString("world"));

    auto lastReq = m_mockTransport->lastRequest();
    QCOMPARE(lastReq.serviceName, QString("Svc"));
    QCOMPARE(lastReq.methodName, QString("method"));
    QVERIFY(!lastReq.requestId.isEmpty());
}

void TestRpc::testClientProxyCallError() {
    m_mockTransport->setError(sc::Error(sc::ErrorCode::NetworkError, "Connection refused"));

    auto result = m_client->call("Svc", "method");
    QVERIFY(result.isErr());
}

void TestRpc::testClientProxyCallParse() {
    QTEST_SET_MAIN_SOURCE_PATH

    sc::rpc::RpcResponse resp;
    resp.success = true;
    resp.data["result"] = "hello";
    m_mockTransport->setResponse(resp);

    auto result = m_client->callAndParse<QString>("Svc", "method");
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap(), QString("hello"));
}

void TestRpc::testClientProxyCallParse_data() {
}

void TestRpc::testServiceRegistryRegister() {
    sc::rpc::InMemoryServiceRegistry registry;
    sc::rpc::ServiceInstance inst;
    inst.serviceName = "OrderService";
    inst.host = "127.0.0.1";
    inst.port = 8080;
    inst.timestamp = QDateTime::currentMSecsSinceEpoch();

    auto result = registry.registerInstance(inst);
    QVERIFY(result.isOk());
}

void TestRpc::testServiceRegistryUnregister() {
    sc::rpc::InMemoryServiceRegistry registry;
    sc::rpc::ServiceInstance inst;
    inst.serviceName = "OrderService";
    inst.host = "127.0.0.1";
    inst.port = 8080;
    inst.timestamp = QDateTime::currentMSecsSinceEpoch();
    (void)registry.registerInstance(inst);

    auto result = registry.unregisterInstance("OrderService", "127.0.0.1", 8080);
    QVERIFY(result.isOk());

    auto list = registry.getInstances("OrderService");
    QVERIFY(list.isOk());
    QVERIFY(list.unwrap().isEmpty());
}

void TestRpc::testServiceRegistryGet() {
    sc::rpc::InMemoryServiceRegistry registry;
    sc::rpc::ServiceInstance inst;
    inst.serviceName = "UserService";
    inst.host = "localhost";
    inst.port = 9090;
    (void)registry.registerInstance(inst);

    auto result = registry.getInstances("UserService");
    QVERIFY(result.isOk());
    auto list = result.unwrap();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list[0].host, QString("localhost"));
    QCOMPARE(list[0].port, 9090);
}

void TestRpc::testServiceRegistryEmpty() {
    sc::rpc::InMemoryServiceRegistry registry;
    auto result = registry.getInstances("NonExistent");
    QVERIFY(result.isOk());
    QVERIFY(result.unwrap().isEmpty());
}

void TestRpc::testLoadBalancerRoundRobin() {
    sc::rpc::LoadBalancer lb;
    lb.setRoundRobin();

    QList<sc::rpc::ServiceInstance> instances;
    for (int i = 0; i < 3; ++i) {
        sc::rpc::ServiceInstance inst;
        inst.host = QString("host%1").arg(i);
        instances.append(inst);
    }

    auto first = lb.select(instances);
    auto second = lb.select(instances);
    auto third = lb.select(instances);

    QCOMPARE(first.host, QString("host0"));
    QCOMPARE(second.host, QString("host1"));
    QCOMPARE(third.host, QString("host2"));
}

void TestRpc::testLoadBalancerRandom() {
    sc::rpc::LoadBalancer lb;
    lb.setRandom();

    QList<sc::rpc::ServiceInstance> instances;
    for (int i = 0; i < 10; ++i) {
        sc::rpc::ServiceInstance inst;
        inst.host = QString("host%1").arg(i);
        instances.append(inst);
    }

    QSet<QString> selected;
    for (int i = 0; i < 100; ++i) {
        auto inst = lb.select(instances);
        selected.insert(inst.host);
    }
    QVERIFY(selected.size() > 1);
}

void TestRpc::testRpcValueSerialization() {
    sc::rpc::RpcValue v1 = qint64(42);
    auto bytes1 = m_serializer->serializeValue(v1);
    auto v1back = m_serializer->deserializeValue(bytes1);
    QCOMPARE(std::get<qint64>(v1back), qint64(42));

    sc::rpc::RpcValue v2 = QString("hello");
    auto bytes2 = m_serializer->serializeValue(v2);
    auto v2back = m_serializer->deserializeValue(bytes2);
    QCOMPARE(std::get<QString>(v2back), QString("hello"));

    sc::rpc::RpcValue v3 = true;
    auto bytes3 = m_serializer->serializeValue(v3);
    auto v3back = m_serializer->deserializeValue(bytes3);
    QCOMPARE(std::get<bool>(v3back), true);

    sc::rpc::RpcValue v4 = 3.14;
    auto bytes4 = m_serializer->serializeValue(v4);
    auto v4back = m_serializer->deserializeValue(bytes4);
    QVERIFY(qAbs(std::get<double>(v4back) - 3.14) < 0.001);
}

QTEST_MAIN(TestRpc)
#include "test_rpc.moc"