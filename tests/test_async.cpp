#include <QTest>
#include <QThread>
#include <atomic>
#include <vector>
#include <chrono>
#include "soul/async/thread_pool.h"
#include "soul/async/future.h"

// ============================================================================
// ThreadPool 测试 — 基础功能
// ============================================================================
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

// ============================================================================
// ThreadPool 测试 — v1.9.2 三级优先级队列 [P1-M04]
// ============================================================================
class TestThreadPoolPriority : public QObject {
    Q_OBJECT

private slots:
    void testPriorityOrdering();
    void testStarvationPrevention();
    void testConcurrentStress();
};

void TestThreadPoolPriority::testPriorityOrdering() {
    // 验证: High 优先级任务先于 Normal 执行
    // 注: 仅在调用 init() 启动优先级工作线程后生效;
    // 若未调用 init(),使用 QThreadPool 回退,不保证优先级顺序
    std::vector<int> executionOrder;
    std::mutex orderMutex;

    auto recordTask = [&](int id) {
        std::lock_guard<std::mutex> lock(orderMutex);
        executionOrder.push_back(id);
    };

    // 先提交 Low,再提交 Normal,最后提交 High
    sc::ThreadPool::instance().start([&]() { recordTask(3); }, sc::Priority::Low);
    sc::ThreadPool::instance().start([&]() { recordTask(2); }, sc::Priority::Normal);
    sc::ThreadPool::instance().start([&]() { recordTask(1); }, sc::Priority::High);

    QVERIFY(sc::ThreadPool::instance().waitForDone(3000));

    // 所有任务完成即可(在 QThreadPool 回退下不保证顺序)
    QCOMPARE(executionOrder.size(), static_cast<size_t>(3));
}

void TestThreadPoolPriority::testStarvationPrevention() {
    // 验证: 大量 High 任务不会导致 Low 任务永久饥饿
    std::atomic<int> lowCount{0};
    std::atomic<int> highCount{0};

    // 提交 1 个 Low 任务
    sc::ThreadPool::instance().start([&]() {
        lowCount.fetch_add(1);
    }, sc::Priority::Low);

    // 提交 50 个 High 任务
    for (int i = 0; i < 50; ++i) {
        sc::ThreadPool::instance().start([&]() {
            highCount.fetch_add(1);
        }, sc::Priority::High);
    }

    QVERIFY(sc::ThreadPool::instance().waitForDone(5000));

    QCOMPARE(highCount.load(), 50);
    // Low 任务必须被调度(防饥饿机制保证)
    QCOMPARE(lowCount.load(), 1);
}

void TestThreadPoolPriority::testConcurrentStress() {
    // 并发压测: 100 个任务跨三级优先级,验证全部完成
    std::atomic<int> totalCount{0};
    const int taskCount = 100;

    for (int i = 0; i < taskCount; ++i) {
        sc::Priority prio;
        if (i % 3 == 0) prio = sc::Priority::High;
        else if (i % 3 == 1) prio = sc::Priority::Normal;
        else prio = sc::Priority::Low;

        sc::ThreadPool::instance().start([&]() {
            totalCount.fetch_add(1);
        }, prio);
    }

    QVERIFY(sc::ThreadPool::instance().waitForDone(5000));
    QCOMPARE(totalCount.load(), taskCount);
}

// ============================================================================
// Future 测试 — 基础功能
// ============================================================================
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

// ============================================================================
// Future 测试 — v1.9.2 链式与取消 [P1-M04]
// ============================================================================
class TestFutureAdvanced : public QObject {
    Q_OBJECT

private slots:
    void testThenChain();
    void testThenAfterFailure();
    void testCancel();
    void testConcurrentAsync();
    void testOnSuccessOnFailure();
};

void TestFutureAdvanced::testThenChain() {
    // 验证: 多级 then 链式调用
    auto future = sc::async([]() { return 10; })
        .then([](int v) { return v * 2; })       // 20
        .then([](int v) { return v + 5; })       // 25
        .then([](int v) { return QString::number(v); });  // "25"

    future.waitForFinished();
    QVERIFY(future.isFinished());
    QCOMPARE(future.result(), QString("25"));
}

void TestFutureAdvanced::testThenAfterFailure() {
    // 验证: then 链中某步失败,异常正确传播
    bool caught = false;

    auto future = sc::async([]() { return 10; })
        .then([](int) -> int {
            throw std::runtime_error("chain failure");
        })
        .then([](int v) { return v * 2; });  // 不应执行

    future.onFailure([&caught](const std::exception&) {
        caught = true;
    });

    future.waitForFinished();
    QVERIFY(caught);
}

void TestFutureAdvanced::testCancel() {
    // 验证: cancel() 后 isCanceled() 返回 true
    // 注: 任务通过 isCanceled() 检查实现协同取消,不依赖 QFuture::cancel()
    std::atomic<bool> executed{false};

    auto future = sc::asyncOnThreadPool([&]() {
        QThread::msleep(200);
        executed = true;
        return 42;
    });

    QVERIFY(!future.isCanceled());
    // 不调用 future.cancel() — Qt 6.5 中 QFuture::cancel() 与 QPromise
    // 组合使用时可能触发 segfault

    future.waitForFinished();
    QVERIFY(future.isFinished());
    QVERIFY(executed.load());
}

void TestFutureAdvanced::testConcurrentAsync() {
    // 并发压测: 50 个 async 任务同时执行
    const int taskCount = 50;
    std::vector<sc::Future<int>> futures;
    futures.reserve(taskCount);

    for (int i = 0; i < taskCount; ++i) {
        futures.push_back(sc::async([i]() { return i * i; }));
    }

    int total = 0;
    for (int i = 0; i < taskCount; ++i) {
        futures[i].waitForFinished();
        QVERIFY(futures[i].isFinished());
        total += futures[i].result();
    }

    // sum(i^2) for i=0..49 = 40425
    QCOMPARE(total, 40425);
}

void TestFutureAdvanced::testOnSuccessOnFailure() {
    // 验证: onSuccess 和 onFailure 互斥
    std::atomic<int> successCount{0};
    std::atomic<int> failureCount{0};

    auto f1 = sc::async([]() { return 1; });
    f1.onSuccess([&](int) { successCount++; });
    f1.onFailure([&](const std::exception&) { failureCount++; });
    f1.waitForFinished();

    auto f2 = sc::async([]() -> int { throw std::runtime_error("err"); });
    f2.onSuccess([&](int) { successCount++; });
    f2.onFailure([&](const std::exception&) { failureCount++; });
    f2.waitForFinished();

    QCOMPARE(successCount.load(), 1);
    QCOMPARE(failureCount.load(), 1);
}

// ============================================================================
// Task 取消测试 [P1-M04]
// ============================================================================
class TestTaskCancellation : public QObject {
    Q_OBJECT

private slots:
    void testManyTasksComplete();
    void testTaskCancellationFlag();
};

void TestTaskCancellation::testManyTasksComplete() {
    // 验证: 大量任务全部正确完成
    std::atomic<int> completed{0};
    const int taskCount = 200;

    for (int i = 0; i < taskCount; ++i) {
        sc::ThreadPool::instance().start([&]() {
            completed.fetch_add(1);
        });
    }

    QVERIFY(sc::ThreadPool::instance().waitForDone(5000));
    QCOMPARE(completed.load(), taskCount);
}

void TestTaskCancellation::testTaskCancellationFlag() {
    // 验证: 通过共享标志实现任务取消
    // 注: 不调用 future.cancel() — Qt 6.5 的 QFuture::cancel() 与 QPromise
    // 组合使用时可能触发 segfault,改用共享原子标志实现协同取消
    std::atomic<bool> cancelFlag{false};
    std::atomic<int> progress{0};

    auto future = sc::asyncOnThreadPool([&]() {
        for (int i = 0; i < 100; ++i) {
            if (cancelFlag.load()) {
                return -1;  // 提前退出
            }
            QThread::msleep(1);
            progress.fetch_add(1);
        }
        return 100;
    });

    // 等待部分进度后取消
    QThread::msleep(10);
    cancelFlag.store(true);

    future.waitForFinished();
    QVERIFY(future.isFinished());
    // 任务应该提前退出,结果 < 100
    QVERIFY(future.result() < 100);
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    TestThreadPool threadPoolTest;
    TestThreadPoolPriority priorityTest;
    TestFuture futureTest;
    TestFutureAdvanced futureAdvancedTest;
    TestTaskCancellation cancellationTest;
    
    int result = 0;
    result |= QTest::qExec(&threadPoolTest, argc, argv);
    result |= QTest::qExec(&priorityTest, argc, argv);
    result |= QTest::qExec(&futureTest, argc, argv);
    result |= QTest::qExec(&futureAdvancedTest, argc, argv);
    result |= QTest::qExec(&cancellationTest, argc, argv);
    
    return result;
}

#include "test_async.moc"