// ============================================================================
// test_websocket_server.cpp — WebSocket Server 单元测试
// ============================================================================
//
// 覆盖范围:
//   - WebSocketServer 启停/监听
//   - HTTP Upgrade 握手
//   - WebSocket 文本消息发送/接收
//   - WebSocket 二进制消息发送/接收
//   - Ping/Pong 自动回复
//   - 广播消息
//   - 会话管理(activeSessionCount)

#include <QTest>
#include <QHostAddress>
#include <QWebSocket>
#include <QSignalSpy>
#include <QTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>

#include "soul/server/websocket_server.h"

using namespace sc;
using namespace sc::server;

// ============================================================================
// TestWebSocketServer — WebSocket Server 测试
// ============================================================================
class TestWebSocketServer : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void testListenAndClose();
    void testWebSocketEcho();
    void testWebSocketBinary();
    void testPingPong();
    void testBroadcast();
    void testSessionCount();
    void testCloseFrame();
};

void TestWebSocketServer::initTestCase() {
}

void TestWebSocketServer::cleanupTestCase() {
}

void TestWebSocketServer::cleanup() {
    QCoreApplication::processEvents();
}

// 验证监听与关闭
void TestWebSocketServer::testListenAndClose() {
    WebSocketServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QVERIFY(server.isListening());
    QVERIFY(server.serverPort() > 0);
    server.close();
    QVERIFY(!server.isListening());
}

// 验证 WebSocket 文本消息 Echo
void TestWebSocketServer::testWebSocketEcho() {
    WebSocketServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    // 设置消息回调: Echo
    QByteArray received;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    server.setOnMessage([&](WebSocketSession* session, const QByteArray& msg, bool isText) {
        received = msg;
        QVERIFY(isText);
        session->send("Echo: " + msg, true);
        // server callback should NOT quit the loop — let the client callback do it
    });

    // 客户端连接
    QWebSocket client;
    QSignalSpy connectedSpy(&client, &QWebSocket::connected);
    client.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
    QVERIFY(connectedSpy.wait(3000));

    // 等待 onOpen 回调
    QCoreApplication::processEvents();

    // 发送消息
    QByteArray echoResponse;
    QObject::connect(&client, &QWebSocket::textMessageReceived,
                     [&](const QString& msg) {
                         echoResponse = msg.toUtf8();
                         loop.quit();
                     });
    client.sendTextMessage("Hello WebSocket");

    // 等待响应
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(received, QByteArray("Hello WebSocket"));
    QCOMPARE(echoResponse, QByteArray("Echo: Hello WebSocket"));

    client.close();
    server.close();
}

// 验证 WebSocket 二进制消息
void TestWebSocketServer::testWebSocketBinary() {
    WebSocketServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    QByteArray binaryReceived;
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    server.setOnMessage([&](WebSocketSession* session, const QByteArray& msg, bool isText) {
        binaryReceived = msg;
        QVERIFY(!isText);  // 二进制消息
        session->sendBinary(msg);  // Echo 二进制
        // server callback should NOT quit the loop — let the client callback do it
    });

    QWebSocket client;
    QSignalSpy connectedSpy(&client, &QWebSocket::connected);
    client.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
    QVERIFY(connectedSpy.wait(3000));
    QCoreApplication::processEvents();

    QByteArray binaryResponse;
    QObject::connect(&client, &QWebSocket::binaryMessageReceived,
                     [&](const QByteArray& msg) {
                         binaryResponse = msg;
                         loop.quit();
                     });

    QByteArray testData = QByteArray("\x00\x01\x02\x03\xFF\xFE\xFD", 7);
    client.sendBinaryMessage(testData);

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(binaryReceived, testData);
    QCOMPARE(binaryResponse, testData);

    client.close();
    server.close();
}

// 验证 Ping/Pong 自动回复
void TestWebSocketServer::testPingPong() {
    WebSocketServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    QWebSocket client;
    QSignalSpy connectedSpy(&client, &QWebSocket::connected);
    QSignalSpy pongSpy(&client, &QWebSocket::pong);

    client.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
    QVERIFY(connectedSpy.wait(3000));
    QCoreApplication::processEvents();

    // 发送 Ping
    client.ping("ping-data");

    // 等待 Pong 响应
    QVERIFY(pongSpy.wait(3000));
    QCOMPARE(pongSpy.count(), 1);

    client.close();
    server.close();
}

// 验证广播消息
void TestWebSocketServer::testBroadcast() {
    WebSocketServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);

    // 两个客户端
    QWebSocket client1, client2;
    QString msg1, msg2;

    QObject::connect(&client1, &QWebSocket::textMessageReceived,
                     [&](const QString& msg) { msg1 = msg; });
    QObject::connect(&client2, &QWebSocket::textMessageReceived,
                     [&](const QString& msg) { msg2 = msg; });

    // 使用计数器等待两个客户端都连接
    int connectedCount = 0;
    QObject::connect(&client1, &QWebSocket::connected, [&]() {
        if (++connectedCount >= 2) loop.quit();
    });
    QObject::connect(&client2, &QWebSocket::connected, [&]() {
        if (++connectedCount >= 2) loop.quit();
    });

    client1.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
    client2.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));

    loop.exec();
    QVERIFY(connectedCount == 2);

    // 广播消息
    server.broadcast("Broadcast Message", true);

    // 等待消息到达
    QEventLoop msgLoop;
    int msgCount = 0;
    QObject::connect(&client1, &QWebSocket::textMessageReceived, [&](const QString&) {
        if (++msgCount >= 2) msgLoop.quit();
    });
    QObject::connect(&client2, &QWebSocket::textMessageReceived, [&](const QString&) {
        if (++msgCount >= 2) msgLoop.quit();
    });
    QTimer::singleShot(3000, &msgLoop, &QEventLoop::quit);
    msgLoop.exec();

    QCOMPARE(msg1, QString("Broadcast Message"));
    QCOMPARE(msg2, QString("Broadcast Message"));

    client1.close();
    client2.close();
    server.close();
}

// 验证会话计数
void TestWebSocketServer::testSessionCount() {
    WebSocketServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    QCOMPARE(server.activeSessionCount(), size_t(0));

    QWebSocket client1, client2;
    QSignalSpy spy1(&client1, &QWebSocket::connected);
    QSignalSpy spy2(&client2, &QWebSocket::connected);

    client1.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
    QVERIFY(spy1.wait(3000));
    QCoreApplication::processEvents();
    QCOMPARE(server.activeSessionCount(), size_t(1));

    client2.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
    QVERIFY(spy2.wait(3000));
    QCoreApplication::processEvents();
    QCOMPARE(server.activeSessionCount(), size_t(2));

    client1.close();
    QTest::qWait(200);
    QCOMPARE(server.activeSessionCount(), size_t(1));

    client2.close();
    QTest::qWait(200);
    QCOMPARE(server.activeSessionCount(), size_t(0));

    server.close();
}

// 验证 Close Frame 处理
void TestWebSocketServer::testCloseFrame() {
    WebSocketServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    bool closeCalled = false;
    server.setOnClose([&closeCalled](WebSocketSession*) {
        closeCalled = true;
    });

    QWebSocket client;
    QSignalSpy connectedSpy(&client, &QWebSocket::connected);
    client.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
    QVERIFY(connectedSpy.wait(3000));
    QCoreApplication::processEvents();

    // 客户端主动关闭
    client.close();
    QTest::qWait(500);

    QVERIFY(closeCalled);

    server.close();
}

QTEST_GUILESS_MAIN(TestWebSocketServer)
#include "test_websocket_server.moc"