// ============================================================================
// test_connection_manager.cpp — ConnectionManager 单元测试
// ============================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimer>

#include "soul/network/connection_manager.h"
#include "soul/network/core/inetwork.h"
#include "soul/network/core/network_state.h"
#include "soul/network/core/network_message.h"

using namespace sc;
using namespace sc::network;

// ============================================================================
// MockNetwork — 可控的 INetwork 模拟实现
// ============================================================================

class MockNetwork : public INetwork {
public:
    explicit MockNetwork(bool initiallyConnected = false)
        : m_connected(initiallyConnected)
    {
    }

    void connectTo(const QUrl& url) override
    {
        m_url = url;
        m_connectCount++;
        m_connected = true;
        m_currentState = NetworkState::Connected;
    }

    void disconnect() override
    {
        m_connected = false;
        m_disconnectCount++;
        m_currentState = NetworkState::Disconnected;
    }

    bool isConnected() const override
    {
        return m_connected;
    }

    Result<NetworkMessage> send(const NetworkMessage& /*message*/) override
    {
        return Result<NetworkMessage>::ok(NetworkMessage());
    }

    void sendAsync(const NetworkMessage& /*message*/, ResponseCallback /*callback*/) override
    {
    }

    NetworkState state() const override
    {
        return m_currentState;
    }

    void setPolicy(std::shared_ptr<INetworkPolicy> /*policy*/) override
    {
    }

    void addInterceptor(std::shared_ptr<IInterceptor<NetworkMessage, NetworkMessage>> /*interceptor*/) override
    {
    }

    // 测试辅助方法
    void simulateDisconnect()
    {
        m_connected = false;
        m_currentState = NetworkState::Disconnected;
    }

    void simulateReconnect()
    {
        m_connected = true;
        m_currentState = NetworkState::Connected;
    }

    int connectCount() const { return m_connectCount; }
    int disconnectCount() const { return m_disconnectCount; }
    QUrl lastUrl() const { return m_url; }

private:
    bool m_connected = false;
    NetworkState m_currentState = NetworkState::Disconnected;
    int m_connectCount = 0;
    int m_disconnectCount = 0;
    QUrl m_url;
};

// ============================================================================
// TestConnectionManager
// ============================================================================

class TestConnectionManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // 确保 QCoreApplication 实例存在
        if (!QCoreApplication::instance()) {
            static int argc = 0;
            static char* argv[] = {nullptr};
            static QCoreApplication app(argc, argv);
        }
    }

    // === 注册/注销 ===

    void testRegisterConnection()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();
        ConnectionConfig config;
        config.url = QUrl("tcp://localhost:8080");
        config.name = "test";

        mgr.registerConnection("test", network, config);

        QCOMPARE(mgr.state("test"), ManagedConnectionState::Disconnected);
        auto names = mgr.connectionNames();
        QCOMPARE(names.size(), size_t(1));
        QCOMPARE(names[0], std::string("test"));
    }

    void testRegisterDuplicateName()
    {
        ConnectionManager mgr;

        auto net1 = std::make_shared<MockNetwork>();
        auto net2 = std::make_shared<MockNetwork>();

        mgr.registerConnection("dup", net1);
        mgr.registerConnection("dup", net2); // 覆盖

        auto names = mgr.connectionNames();
        QCOMPARE(names.size(), size_t(1));
    }

    void testUnregisterConnection()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();
        mgr.registerConnection("test", network);
        mgr.unregisterConnection("test");

        auto names = mgr.connectionNames();
        QCOMPARE(names.size(), size_t(0));
    }

    void testRegisterNullNetwork()
    {
        ConnectionManager mgr;
        mgr.registerConnection("null", nullptr);
        QCOMPARE(mgr.connectionNames().size(), size_t(0));
    }

    // === 连接/断开 ===

    void testConnectAndDisconnect()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();
        ConnectionConfig config;
        config.url = QUrl("tcp://localhost:8080");
        mgr.registerConnection("test", network, config);

        QSignalSpy spy(&mgr, &ConnectionManager::connectionStateChanged);

        mgr.connect("test");
        QCoreApplication::processEvents();

        // 轮询检测到连接后,状态应变为 Connected
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("test"), ManagedConnectionState::Connected, 3000);

        QVERIFY(spy.count() >= 1); // Disconnected → Connecting → Connected

        mgr.disconnect("test");
        QCOMPARE(mgr.state("test"), ManagedConnectionState::Disconnected);
    }

    void testConnectAll()
    {
        ConnectionManager mgr;

        auto net1 = std::make_shared<MockNetwork>();
        auto net2 = std::make_shared<MockNetwork>();

        mgr.registerConnection("c1", net1, ConnectionConfig{QUrl("tcp://localhost:8081")});
        mgr.registerConnection("c2", net2, ConnectionConfig{QUrl("tcp://localhost:8082")});

        mgr.connectAll();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("c1"), ManagedConnectionState::Connected, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("c2"), ManagedConnectionState::Connected, 3000);

        QCOMPARE(mgr.activeConnectionCount(), size_t(2));
    }

    void testDisconnectAll()
    {
        ConnectionManager mgr;

        auto net1 = std::make_shared<MockNetwork>();
        auto net2 = std::make_shared<MockNetwork>();

        mgr.registerConnection("c1", net1, ConnectionConfig{QUrl("tcp://localhost:8081")});
        mgr.registerConnection("c2", net2, ConnectionConfig{QUrl("tcp://localhost:8082")});

        mgr.connectAll();
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.activeConnectionCount(), size_t(2), 3000);

        mgr.disconnectAll();

        QCOMPARE(mgr.state("c1"), ManagedConnectionState::Disconnected);
        QCOMPARE(mgr.state("c2"), ManagedConnectionState::Disconnected);
        QCOMPARE(mgr.activeConnectionCount(), size_t(0));
    }

    void testConnectAlreadyConnected()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();
        mgr.registerConnection("test", network, ConnectionConfig{QUrl("tcp://localhost:8080")});

        mgr.connect("test");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("test"), ManagedConnectionState::Connected, 3000);

        int connectCountBefore = network->connectCount();

        // 再次连接应该被忽略
        mgr.connect("test");
        QCoreApplication::processEvents();

        QCOMPARE(network->connectCount(), connectCountBefore);
    }

    // === 状态查询 ===

    void testStateQuery()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();
        mgr.registerConnection("test", network, ConnectionConfig{QUrl("tcp://localhost:8080")});

        QCOMPARE(mgr.state("test"), ManagedConnectionState::Disconnected);
        QCOMPARE(mgr.isConnected("test"), false);
        QCOMPARE(mgr.activeConnectionCount(), size_t(0));

        mgr.connect("test");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.isConnected("test"), true, 3000);

        QCOMPARE(mgr.activeConnectionCount(), size_t(1));
    }

    void testStateQueryUnknownName()
    {
        ConnectionManager mgr;
        QCOMPARE(mgr.state("unknown"), ManagedConnectionState::Disconnected);
        QCOMPARE(mgr.isConnected("unknown"), false);
    }

    // === 断线重连(自动) ===

    void testAutoReconnectOnDisconnect()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();

        ConnectionConfig config;
        config.url = QUrl("tcp://localhost:9090");
        config.autoReconnect = true;
        config.baseInterval = std::chrono::milliseconds(100);
        config.maxRetries = 10;

        mgr.registerConnection("reconn", network, config);

        QSignalSpy spy(&mgr, &ConnectionManager::connectionStateChanged);

        mgr.connect("reconn");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("reconn"), ManagedConnectionState::Connected, 3000);

        // 模拟断线
        network->simulateDisconnect();

        // 轮询检查到断线 → 触发重连
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("reconn"), ManagedConnectionState::Reconnecting, 5000);

        // 模拟重连成功
        network->simulateReconnect();

        // 轮询检测到重连成功
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("reconn"), ManagedConnectionState::Connected, 5000);

        // 验证状态变化序列包含 Reconnecting 状态
        bool hasReconnecting = false;
        for (int i = 0; i < spy.count(); ++i) {
            auto args = spy[i];
            auto state = args[1].value<ManagedConnectionState>();
            if (state == ManagedConnectionState::Reconnecting) {
                hasReconnecting = true;
                break;
            }
        }
        QVERIFY(hasReconnecting);
    }

    void testMaxRetriesExceeded()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();

        ConnectionConfig config;
        config.url = QUrl("tcp://localhost:9090");
        config.autoReconnect = true;
        config.baseInterval = std::chrono::milliseconds(50);
        config.maxRetries = 2; // 仅允许 2 次重试

        mgr.registerConnection("limited", network, config);

        mgr.connect("limited");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("limited"), ManagedConnectionState::Connected, 3000);

        // 第一次断线
        network->simulateDisconnect();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("limited"), ManagedConnectionState::Reconnecting, 3000);

        // 重连成功(第 1 次重试)
        network->simulateReconnect();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("limited"), ManagedConnectionState::Connected, 3000);

        // 第二次断线
        network->simulateDisconnect();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("limited"), ManagedConnectionState::Reconnecting, 3000);

        // 重连成功(第 2 次重试,达到上限)
        network->simulateReconnect();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("limited"), ManagedConnectionState::Connected, 3000);

        // 第三次断线 → 超过 maxRetries(2) → Error
        network->simulateDisconnect();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("limited"), ManagedConnectionState::Error, 5000);
    }

    void testNoAutoReconnect()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();

        ConnectionConfig config;
        config.url = QUrl("tcp://localhost:9090");
        config.autoReconnect = false;

        mgr.registerConnection("noreconn", network, config);

        mgr.connect("noreconn");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("noreconn"), ManagedConnectionState::Connected, 3000);

        network->simulateDisconnect();

        // 等待一段时间的轮询后,应该停留在 Disconnected,不会进入 Reconnecting
        QTest::qWait(1000);
        QCoreApplication::processEvents();

        QCOMPARE(mgr.state("noreconn"), ManagedConnectionState::Disconnected);
    }

    // === StateListener 回调通知 ===

    void testStateListenerNotification()
    {
        std::string lastName;
        ManagedConnectionState lastState = ManagedConnectionState::Disconnected;
        bool received = false;

        auto listener = [&](const std::string& name,
                            ManagedConnectionState state,
                            ManagedConnectionState /*previousState*/,
                            const std::string& /*message*/) {
            lastName = name;
            lastState = state;
            received = true;
        };

        ConnectionManager mgr(listener);

        auto network = std::make_shared<MockNetwork>();
        mgr.registerConnection("sl", network, ConnectionConfig{QUrl("tcp://localhost:8080")});

        mgr.connect("sl");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("sl"), ManagedConnectionState::Connected, 3000);

        QCoreApplication::processEvents();
        QTest::qWait(50);

        QVERIFY(received);
        QCOMPARE(lastName, std::string("sl"));
        QVERIFY(lastState == ManagedConnectionState::Connected || lastState == ManagedConnectionState::Connecting);
    }

    // === 指数退避算法 ===

    void testExponentialBackoff()
    {
        ConnectionConfig config;
        config.baseInterval = std::chrono::milliseconds(1000);
        config.maxInterval = std::chrono::milliseconds(60000);

        // retryCount=0: 1000ms * 2^0 ≈ 1000ms
        auto interval0 = ConnectionManager::nextRetryInterval(0, config);
        QVERIFY(interval0.count() >= 100); // 最小 100ms
        QVERIFY(interval0.count() <= 2000); // 带抖动

        // retryCount=5: 1000ms * 2^5 = 32000ms
        auto interval5 = ConnectionManager::nextRetryInterval(5, config);
        QVERIFY(interval5.count() >= 16000);
        QVERIFY(interval5.count() <= 48000); // ±25% 抖动

        // retryCount=10: 1000ms * 2^10 = 1024000ms → cap at maxInterval(60000ms)
        auto interval10 = ConnectionManager::nextRetryInterval(10, config);
        QVERIFY(interval10.count() <= 90000); // maxInterval * 1.25 抖动上限
    }

    // === 心跳超时 ===

    void testHeartbeatTimeout()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();

        ConnectionConfig config;
        config.url = QUrl("tcp://localhost:8080");
        config.autoReconnect = true;
        config.enableHeartbeat = true;
        config.heartbeatIntervalMs = 1000;
        config.heartbeatTimeoutMs = 500;
        config.baseInterval = std::chrono::milliseconds(100);

        mgr.registerConnection("hb", network, config);

        mgr.connect("hb");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("hb"), ManagedConnectionState::Connected, 3000);

        // 等待心跳超时 → 断开 → 重连
        // 心跳超时时间为 500ms,等待足够长时间
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("hb"), ManagedConnectionState::Reconnecting, 5000);

        // 验证网络被断开
        QVERIFY(network->disconnectCount() >= 1);
    }

    // === 连接名称 ===

    void testConnectionNames()
    {
        ConnectionManager mgr;

        auto net1 = std::make_shared<MockNetwork>();
        auto net2 = std::make_shared<MockNetwork>();
        auto net3 = std::make_shared<MockNetwork>();

        mgr.registerConnection("alpha", net1);
        mgr.registerConnection("beta", net2);
        mgr.registerConnection("gamma", net3);

        auto names = mgr.connectionNames();
        QCOMPARE(names.size(), size_t(3));

        // 验证所有名称都存在
        auto hasName = [&](const std::string& n) {
            return std::find(names.begin(), names.end(), n) != names.end();
        };
        QVERIFY(hasName("alpha"));
        QVERIFY(hasName("beta"));
        QVERIFY(hasName("gamma"));
    }

    void testActiveConnectionCount()
    {
        ConnectionManager mgr;

        auto net1 = std::make_shared<MockNetwork>();
        auto net2 = std::make_shared<MockNetwork>();

        mgr.registerConnection("c1", net1, ConnectionConfig{QUrl("tcp://localhost:8081")});
        mgr.registerConnection("c2", net2, ConnectionConfig{QUrl("tcp://localhost:8082")});

        QCOMPARE(mgr.activeConnectionCount(), size_t(0));

        mgr.connect("c1");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("c1"), ManagedConnectionState::Connected, 3000);
        QCOMPARE(mgr.activeConnectionCount(), size_t(1));

        mgr.connect("c2");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("c2"), ManagedConnectionState::Connected, 3000);
        QCOMPARE(mgr.activeConnectionCount(), size_t(2));

        mgr.disconnect("c1");
        QCOMPARE(mgr.activeConnectionCount(), size_t(1));
    }

    // === 状态转换信号 ===

    void testStateChangeSignal()
    {
        ConnectionManager mgr;

        auto network = std::make_shared<MockNetwork>();
        mgr.registerConnection("sig", network, ConnectionConfig{QUrl("tcp://localhost:8080")});

        QSignalSpy spy(&mgr, &ConnectionManager::connectionStateChanged);

        mgr.connect("sig");
        QCoreApplication::processEvents();
        QTRY_COMPARE_WITH_TIMEOUT(mgr.state("sig"), ManagedConnectionState::Connected, 3000);

        QVERIFY(spy.count() >= 1);

        // 验证信号参数
        bool foundConnected = false;
        for (int i = 0; i < spy.count(); ++i) {
            auto args = spy[i];
            auto name = args[0].value<std::string>();
            auto state = args[1].value<ManagedConnectionState>();

            if (name == "sig" && state == ManagedConnectionState::Connected) {
                foundConnected = true;
                break;
            }
        }
        QVERIFY(foundConnected);
    }
};

QTEST_MAIN(TestConnectionManager)
#include "test_connection_manager.moc"