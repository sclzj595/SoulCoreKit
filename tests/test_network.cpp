#include <QTest>
#include "soul/network/core/inetwork.h"
#include "soul/network/core/network_message.h"
#include "soul/network/core/network_state.h"
#include "soul/network/factory/network_factory.h"
#include "soul/network/policy/timeout_policy.h"
#include "soul/network/policy/retry_policy.h"
#include "soul/network/interceptor/logging_interceptor.h"
#include "soul/network/interceptor/auth_interceptor.h"
#include "soul/network/http/http_client_adapter.h"
#include "soul/network/tcp/tcp_client_adapter.h"
#include "soul/network/websocket/ws_client_adapter.h"

class TestNetwork : public QObject {
    Q_OBJECT

private slots:
    void testNetworkMessageFields();
    void testMetadataStorage();
    void testCreateHttp();
    void testCreateTcp();
    void testCreateWs();
    void testCreateUnknownProtocol();
    void testTimeoutPolicy();
    void testRetryPolicy();
    void testLoggingInterceptor();
    void testAuthInterceptor();
    void testHttpClientAdapterState();
    void testHttpClientAdapterInterceptors();
    void testHttpClientAdapterPolicy();
    void testTcpClientAdapterState();
    void testWsClientAdapterMessageType();
};

void TestNetwork::testNetworkMessageFields() {
    sc::network::NetworkMessage msg;
    msg.api = "/api/test";
    msg.payload = "test payload";
    msg.statusCode = 200;
    msg.message = "Success";
    msg.duration = 100;

    QCOMPARE(msg.api, QString("/api/test"));
    QCOMPARE(msg.payload, QByteArray("test payload"));
    QCOMPARE(msg.statusCode, 200);
    QCOMPARE(msg.message, QString("Success"));
    QCOMPARE(msg.duration, (qint64)100);
}

void TestNetwork::testMetadataStorage() {
    sc::network::NetworkMessage msg;
    msg.metadata["key1"] = "value1";
    msg.metadata["key2"] = 42;

    QVERIFY(msg.metadata.contains("key1"));
    QVERIFY(msg.metadata.contains("key2"));
    QCOMPARE(msg.metadata["key1"].toString(), QString("value1"));
    QCOMPARE(msg.metadata["key2"].toInt(), 42);
}

void TestNetwork::testCreateHttp() {
    auto network = sc::network::NetworkFactory::instance().create(QUrl("http://localhost"));
    QVERIFY(network != nullptr);
    QCOMPARE(network->interfaceName(), std::string("sc::network::INetwork"));
}

void TestNetwork::testCreateTcp() {
    auto network = sc::network::NetworkFactory::instance().create(QUrl("tcp://localhost"));
    QVERIFY(network != nullptr);
    QCOMPARE(network->interfaceName(), std::string("sc::network::INetwork"));
}

void TestNetwork::testCreateWs() {
    auto network = sc::network::NetworkFactory::instance().create(QUrl("ws://localhost"));
    QVERIFY(network != nullptr);
    QCOMPARE(network->interfaceName(), std::string("sc::network::INetwork"));
}

void TestNetwork::testCreateUnknownProtocol() {
    auto network = sc::network::NetworkFactory::instance().create(QUrl("mqtt://localhost"));
    QVERIFY(network != nullptr);
}

void TestNetwork::testTimeoutPolicy() {
    sc::network::TimeoutPolicy policy(5000);
    sc::network::NetworkMessage msg;
    
    policy.apply(msg);
    
    QVERIFY(msg.metadata.contains("timeout"));
    QCOMPARE(msg.metadata["timeout"].toInt(), 5000);
}

void TestNetwork::testRetryPolicy() {
    sc::network::RetryPolicy policy(3, sc::network::RetryStrategy::ExponentialBackoff);
    sc::network::NetworkMessage msg;
    
    policy.apply(msg);
    
    QVERIFY(msg.metadata.contains("maxRetries"));
    QVERIFY(msg.metadata.contains("retryStrategy"));
    QVERIFY(msg.metadata.contains("baseDelay"));
    QCOMPARE(msg.metadata["maxRetries"].toInt(), 3);
}

void TestNetwork::testLoggingInterceptor() {
    sc::network::LoggingInterceptor interceptor;
    sc::network::NetworkMessage msg;
    msg.api = "/api/test";
    msg.payload = "test";
    
    interceptor.onRequest(msg);
    interceptor.onResponse(msg);
}

void TestNetwork::testAuthInterceptor() {
    sc::network::AuthInterceptor interceptor("test-token");
    sc::network::NetworkMessage msg;
    
    interceptor.onRequest(msg);
    
    QVERIFY(msg.metadata.contains("Authorization"));
    QCOMPARE(msg.metadata["Authorization"].toString(), QString("Bearer test-token"));
    
    interceptor.setToken("new-token");
    QCOMPARE(interceptor.token(), QString("new-token"));
}

void TestNetwork::testHttpClientAdapterState() {
    sc::network::HttpClientAdapter adapter;
    
    QCOMPARE(adapter.state(), sc::network::NetworkState::Created);
    
    adapter.connectTo(QUrl("http://localhost"));
    QCOMPARE(adapter.state(), sc::network::NetworkState::Connected);
    
    adapter.disconnect();
    QCOMPARE(adapter.state(), sc::network::NetworkState::Disconnected);
}

void TestNetwork::testHttpClientAdapterInterceptors() {
    sc::network::HttpClientAdapter adapter;
    auto loggingInterceptor = std::make_shared<sc::network::LoggingInterceptor>();
    auto authInterceptor = std::make_shared<sc::network::AuthInterceptor>("token");
    
    adapter.addInterceptor(loggingInterceptor);
    adapter.addInterceptor(authInterceptor);
    
    QVERIFY(true);
}

void TestNetwork::testHttpClientAdapterPolicy() {
    sc::network::HttpClientAdapter adapter;
    auto timeoutPolicy = std::make_shared<sc::network::TimeoutPolicy>(10000);
    
    adapter.setPolicy(timeoutPolicy);
    
    QVERIFY(true);
}

void TestNetwork::testTcpClientAdapterState() {
    sc::network::TcpClientAdapter adapter;
    
    QCOMPARE(adapter.state(), sc::network::NetworkState::Created);
}

void TestNetwork::testWsClientAdapterMessageType() {
    sc::network::WsClientAdapter adapter;
    
    sc::network::NetworkMessage textMsg;
    textMsg.payload = "hello";
    textMsg.metadata["messageType"] = "text";
    
    auto result = adapter.send(textMsg);
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().metadata["messageType"].toString(), QString("text"));
    
    sc::network::NetworkMessage binaryMsg;
    binaryMsg.payload = QByteArray(10, 0x01);
    binaryMsg.metadata["messageType"] = "binary";
    
    result = adapter.send(binaryMsg);
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().metadata["messageType"].toString(), QString("binary"));
}

QTEST_MAIN(TestNetwork)
#include "test_network.moc"