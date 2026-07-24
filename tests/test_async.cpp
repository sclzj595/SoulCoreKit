#include <QTest>
#include "soul/async/thread_pool.h"
#include "soul/async/future.h"

class TestThreadPool : public QObject {
    Q_OBJECT

private slots:
    void testStartTask();
    void testMaxThreadCount();
    void testWaitForDone();
};

void TestThreadPool::testStartTask() {
    bool executed = false;
    
    sc::ThreadPool::instance().start([&executed]() {
        executed = true;
    });
    
    QVERIFY(sc::ThreadPool::instance().waitForDone(2000));
    QVERIFY(executed);
}

void TestThreadPool::testMaxThreadCount() {
    sc::ThreadPool::instance().setMaxThreadCount(4);
    QCOMPARE(sc::ThreadPool::instance().maxThreadCount(), 4);
}

void TestThreadPool::testWaitForDone() {
    bool executed = false;
    
    sc::ThreadPool::instance().start([&executed]() {
        QThread::msleep(100);
        executed = true;
    });
    
    QVERIFY(sc::ThreadPool::instance().waitForDone(2000));
    QVERIFY(executed);
}

class TestFuture : public QObject {
    Q_OBJECT

private slots:
    void testAsync();
    void testAsyncOnThreadPool();
    void testThen();
    void testOnSuccess();
    void testOnFailure();
    void testIsFinished();
};

void TestFuture::testAsync() {
    auto future = sc::async([]() { return 42; });
    
    future.waitForFinished();
    QVERIFY(future.isFinished());
    QCOMPARE(future.result(), 42);
}

void TestFuture::testAsyncOnThreadPool() {
    auto future = sc::asyncOnThreadPool([]() { return QString("hello"); });
    
    future.waitForFinished();
    QVERIFY(future.isFinished());
    QCOMPARE(future.result(), QString("hello"));
}

void TestFuture::testThen() {
    auto future = sc::async([]() { return 42; })
        .then([](int value) { return value * 2; });
    
    future.waitForFinished();
    QCOMPARE(future.result(), 84);
}

void TestFuture::testOnSuccess() {
    int result = 0;
    
    auto future = sc::async([]() { return 42; });
    future.onSuccess([&result](int value) {
        result = value;
    });
    
    future.waitForFinished();
    QCOMPARE(result, 42);
}

void TestFuture::testOnFailure() {
    bool failed = false;
    QString errorMsg;
    
    auto future = sc::async([]() -> int {
        throw std::runtime_error("test error");
        return 0;
    });
    
    future.onFailure([&failed, &errorMsg](const std::exception& e) {
        failed = true;
        errorMsg = QString::fromStdString(e.what());
    });
    
    future.waitForFinished();
    QVERIFY(failed);
    QCOMPARE(errorMsg, QString("test error"));
}

void TestFuture::testIsFinished() {
    auto future = sc::async([]() {
        QThread::msleep(50);
        return true;
    });
    
    QVERIFY(!future.isFinished());
    QVERIFY(future.isRunning());
    
    future.waitForFinished();
    QVERIFY(future.isFinished());
    QVERIFY(!future.isRunning());
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    TestThreadPool threadPoolTest;
    TestFuture futureTest;
    
    int result = 0;
    result |= QTest::qExec(&threadPoolTest, argc, argv);
    result |= QTest::qExec(&futureTest, argc, argv);
    
    return result;
}

#include "test_async.moc"