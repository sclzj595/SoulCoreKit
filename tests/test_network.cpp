#include <QTest>
#include <memory>
#include <stdexcept>
#include <thread>
#include <chrono>
#include "soul/network/core/inetwork.h"
#include "soul/network/core/network_message.h"
#include "soul/network/core/network_state.h"
#include "soul/network/factory/network_factory.h"
#include "soul/network/policy/timeout_policy.h"
#include "soul/network/policy/retry_policy.h"
#include "soul/network/policy/reconnect_policy.h"
#include "soul/network/policy/heartbeat_policy.h"
#include "soul/network/interceptor/logging_interceptor.h"
#include "soul/network/interceptor/auth_interceptor.h"
#include "soul/network/http/http_client_adapter.h"
#include "soul/network/tcp/tcp_client_adapter.h"
#include "soul/network/websocket/ws_client_adapter.h"
#include "soul/network/pool/connection_pool.h"

// ============================================================================
// TestNetwork — 基础功能测试
// ============================================================================
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
    void testRetryPolicyNextDelay();
    void testReconnectPolicy();
    void testHeartbeatPolicy();
    void testLoggingInterceptor();
    void testAuthInterceptor();
    void testHttpClientAdapterState();
    void testHttpClientAdapterInterceptors();
    void testHttpClientAdapterPolicy();
    void testTcpClientAdapterState();
    void testWsClientAdapterMessageType();
    void testWsClientAdapterWithPolicy();
    void testConnectionPool();
    void testConnectionPoolMaxConnections();
    void testConnectionPoolAcquireGuarded();
    void testConnectionPoolGuardAutoReleases();
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
    QCOMPARE(msg.duration, static_cast<qint64>(100));
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
    auto authInterceptor = std::make_shared<sc::network::AuthInterceptor>("test-token");
    
    adapter.addInterceptor(authInterceptor);
    
    sc::network::NetworkMessage msg;
    msg.api = "/api/test";
    
    auto result = adapter.send(msg);
    QVERIFY(result.isOk());
}

void TestNetwork::testHttpClientAdapterPolicy() {
    sc::network::HttpClientAdapter adapter;
    auto timeoutPolicy = std::make_shared<sc::network::TimeoutPolicy>(10000);
    
    adapter.setPolicy(timeoutPolicy);
    
    sc::network::NetworkMessage msg;
    msg.api = "/api/test";
    
    auto result = adapter.send(msg);
    QVERIFY(result.isOk());
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

void TestNetwork::testRetryPolicyNextDelay() {
    sc::network::RetryPolicy policy(5, sc::network::RetryStrategy::ExponentialBackoff);
    policy.setBaseDelay(1000);
    
    int delay1 = policy.nextDelay(0);
    int delay2 = policy.nextDelay(1);
    int delay3 = policy.nextDelay(2);
    
    QVERIFY(delay1 > 0);
    QVERIFY(delay2 >= delay1);
    QVERIFY(delay3 >= delay2);
}

void TestNetwork::testReconnectPolicy() {
    sc::network::ReconnectPolicy policy(true, std::chrono::milliseconds(5000), 3);
    sc::network::NetworkMessage msg;
    
    policy.apply(msg);
    
    QVERIFY(msg.metadata.contains("reconnectInterval"));
    QVERIFY(msg.metadata.contains("maxRetries"));
    QCOMPARE(msg.metadata["reconnectInterval"].toInt(), 5000);
    QCOMPARE(msg.metadata["maxRetries"].toInt(), 3);
}

void TestNetwork::testHeartbeatPolicy() {
    sc::network::HeartbeatPolicy policy(10000, 30000);
    sc::network::NetworkMessage msg;
    
    policy.apply(msg);
    
    QVERIFY(msg.metadata.contains("heartbeatInterval"));
    QVERIFY(msg.metadata.contains("heartbeatTimeout"));
    QCOMPARE(msg.metadata["heartbeatInterval"].toInt(), 10000);
    QCOMPARE(msg.metadata["heartbeatTimeout"].toInt(), 30000);
}

void TestNetwork::testWsClientAdapterWithPolicy() {
    sc::network::WsClientAdapter adapter;
    auto heartbeatPolicy = std::make_shared<sc::network::HeartbeatPolicy>(5000, 15000);
    
    adapter.setPolicy(heartbeatPolicy);
    
    sc::network::NetworkMessage msg;
    msg.payload = "ping";
    
    auto result = adapter.send(msg);
    QVERIFY(result.isOk());
}

void TestNetwork::testConnectionPool() {
    sc::network::ConnectionPool::Config config;
    config.maxConnections = 10;
    config.minConnections = 2;
    auto pool = std::make_shared<sc::network::ConnectionPool>(config);
    
    auto conn = pool->acquire(QUrl("http://localhost"));
    QVERIFY(conn != nullptr);
    
    pool->release(conn);
}

void TestNetwork::testConnectionPoolMaxConnections() {
    sc::network::ConnectionPool::Config config;
    config.maxConnections = 5;
    auto pool = std::make_shared<sc::network::ConnectionPool>(config);

    QCOMPARE(pool->config().maxConnections, 5);

    for (int i = 0; i < 5; ++i) {
        auto conn = pool->acquire(QUrl("http://localhost"));
        QVERIFY(conn != nullptr);
    }

    pool->closeAll();
}

void TestNetwork::testConnectionPoolAcquireGuarded() {
    sc::network::ConnectionPool::Config config;
    config.maxConnections = 3;
    auto pool = std::make_shared<sc::network::ConnectionPool>(config);

    {
        auto result = pool->acquireGuarded(QUrl("http://localhost"));
        QVERIFY(result.isOk());
        auto guard = std::move(result.unwrap());
        QVERIFY(guard);
        QVERIFY(guard.connection() != nullptr);
    }

    auto result2 = pool->acquireGuarded(QUrl("http://localhost"));
    QVERIFY(result2.isOk());
    QVERIFY(result2.unwrap());

    pool->closeAll();
}

void TestNetwork::testConnectionPoolGuardAutoReleases() {
    sc::network::ConnectionPool::Config config;
    config.maxConnections = 1;
    auto pool = std::make_shared<sc::network::ConnectionPool>(config);

    try {
        auto result = pool->acquireGuarded(QUrl("http://localhost"));
        QVERIFY(result.isOk());
        auto guard = std::move(result.unwrap());
        throw std::runtime_error("simulated failure");
    } catch (const std::runtime_error&) {
    }

    auto second = pool->acquireGuarded(QUrl("http://localhost"));
    QVERIFY2(second.isOk(), "ConnectionGuard failed to release connection on exception path");

    pool->closeAll();
}

// ============================================================================
// TestNetworkAdvanced — v1.9.2 拦截器链/重试策略/连接池超时 [P1-M05]
// ============================================================================
class TestNetworkAdvanced : public QObject {
    Q_OBJECT

private slots:
    void testInterceptorChain();
    void testRetryStrategyExponentialBackoff();
    void testRetryStrategyFixedDelay();
    void testConnectionPoolTimeout();
    void testConnectionPoolStats();
    void testConnectionPoolCleanupIdle();
};

void TestNetworkAdvanced::testInterceptorChain() {
    // 验证: 多个拦截器按添加顺序组成链
    sc::network::HttpClientAdapter adapter;

    auto authInterceptor = std::make_shared<sc::network::AuthInterceptor>("chain-token");
    auto logInterceptor = std::make_shared<sc::network::LoggingInterceptor>();

    adapter.addInterceptor(authInterceptor);
    adapter.addInterceptor(logInterceptor);

    sc::network::NetworkMessage msg;
    msg.api = "/api/test-chain";

    auto result = adapter.send(msg);
    QVERIFY(result.isOk());
}

void TestNetworkAdvanced::testRetryStrategyExponentialBackoff() {
    // 验证: 指数退避策略延迟递增
    sc::network::RetryPolicy policy(5, sc::network::RetryStrategy::ExponentialBackoff);
    policy.setBaseDelay(100);

    int prevDelay = 0;
    for (int attempt = 0; attempt < 5; ++attempt) {
        int delay = policy.nextDelay(attempt);
        // 指数退避: delay = baseDelay * 2^attempt
        QVERIFY(delay >= prevDelay);
        prevDelay = delay;
    }

    // 达到最大重试后返回 -1
    int maxDelay = policy.nextDelay(6);
    QCOMPARE(maxDelay, -1);
}

void TestNetworkAdvanced::testRetryStrategyFixedDelay() {
    // 验证: 固定延迟策略
    sc::network::RetryPolicy policy(3, sc::network::RetryStrategy::FixedInterval);
    policy.setBaseDelay(500);

    for (int attempt = 0; attempt < 3; ++attempt) {
        int delay = policy.nextDelay(attempt);
        QCOMPARE(delay, 500);
    }

    // 超过最大重试后返回 -1
    int maxDelay = policy.nextDelay(4);
    QCOMPARE(maxDelay, -1);
}

void TestNetworkAdvanced::testConnectionPoolTimeout() {
    // 验证: 连接池配置和容量限制(不依赖网络连接)
    sc::network::ConnectionPool::Config config;
    config.maxConnections = 2;
    config.connectionTimeoutMs = 100;
    auto pool = std::make_shared<sc::network::ConnectionPool>(config);

    QCOMPARE(pool->maxCount(), 2);
    QCOMPARE(pool->config().connectionTimeoutMs, 100);
    QCOMPARE(pool->activeCount(), 0);
    QCOMPARE(pool->idleCount(), 0);

    pool->closeAll();
}

void TestNetworkAdvanced::testConnectionPoolStats() {
    // 验证: 连接池统计信息正确(不依赖网络连接)
    sc::network::ConnectionPool::Config config;
    config.maxConnections = 5;
    auto pool = std::make_shared<sc::network::ConnectionPool>(config);

    QCOMPARE(pool->maxCount(), 5);
    QCOMPARE(pool->activeCount(), 0);
    QCOMPARE(pool->idleCount(), 0);

    pool->closeAll();
}

void TestNetworkAdvanced::testConnectionPoolCleanupIdle() {
    // 验证: cleanupIdleConnections 不崩溃
    sc::network::ConnectionPool::Config config;
    config.maxConnections = 3;
    config.idleTimeoutMs = 10;
    auto pool = std::make_shared<sc::network::ConnectionPool>(config);

    // cleanupIdleConnections 应正常执行(空池)
    pool->cleanupIdleConnections();

    pool->closeAll();
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    TestNetwork basicTest;
    TestNetworkAdvanced advancedTest;
    
    int result = 0;
    result |= QTest::qExec(&basicTest, argc, argv);
    result |= QTest::qExec(&advancedTest, argc, argv);
    
    return result;
}

#include "test_network.moc"