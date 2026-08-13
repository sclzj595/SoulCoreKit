// ============================================================================
// test_lifecycle_basic.cpp — ILifecycle 基本语义测试 [v2.6.0]
// ============================================================================
// 验证:
//   1. ILifecycle 四阶段调用顺序 (initialize → start → stop → shutdown)
//   2. 状态转换正确性 (Constructed → Initialized → Running → Stopped → Shutdown)
//   3. 默认实现 (空操作不崩溃)
//   4. 多次 stop/shutdown 幂等性
//   5. 跳过 start 直接 shutdown
// ============================================================================

#include <QtTest>
#include "soul/core/lifecycle.h"

using namespace sc;

// --- 测试用 Mock 实现 ---

class MockLifecycle : public ILifecycle {
public:
    int initializeCount = 0;
    int startCount = 0;
    int stopCount = 0;
    int shutdownCount = 0;
    LifecycleState currentState = LifecycleState::Constructed;

    bool initializeShouldFail = false;
    bool startShouldFail = false;

    Result<void> initialize() override {
        currentState = LifecycleState::Initializing;
        initializeCount++;
        if (initializeShouldFail) {
            currentState = LifecycleState::Failed;
            return Error(ErrorCode::InternalError, "Mock initialize failure");
        }
        currentState = LifecycleState::Initialized;
        return {};
    }

    Result<void> start() override {
        currentState = LifecycleState::Starting;
        startCount++;
        if (startShouldFail) {
            currentState = LifecycleState::Failed;
            return Error(ErrorCode::InternalError, "Mock start failure");
        }
        currentState = LifecycleState::Running;
        return {};
    }

    void stop() noexcept override {
        currentState = LifecycleState::Stopping;
        stopCount++;
        currentState = LifecycleState::Stopped;
    }

    void shutdown() noexcept override {
        currentState = LifecycleState::ShuttingDown;
        shutdownCount++;
        currentState = LifecycleState::Shutdown;
    }

    LifecycleState state() const override { return currentState; }
};

// --- 测试类 ---

class TestLifecycleBasic : public QObject {
    Q_OBJECT

private slots:
    // 1. 完整四阶段生命周期
    void testFullLifecycle() {
        MockLifecycle mock;
        QCOMPARE(mock.state(), LifecycleState::Constructed);

        auto r1 = mock.initialize();
        QVERIFY(r1.isOk());
        QCOMPARE(mock.state(), LifecycleState::Initialized);
        QCOMPARE(mock.initializeCount, 1);

        auto r2 = mock.start();
        QVERIFY(r2.isOk());
        QCOMPARE(mock.state(), LifecycleState::Running);
        QCOMPARE(mock.startCount, 1);

        mock.stop();
        QCOMPARE(mock.state(), LifecycleState::Stopped);
        QCOMPARE(mock.stopCount, 1);

        mock.shutdown();
        QCOMPARE(mock.state(), LifecycleState::Shutdown);
        QCOMPARE(mock.shutdownCount, 1);
    }

    // 2. initialize 失败 → 状态为 Failed
    void testInitializeFailure() {
        MockLifecycle mock;
        mock.initializeShouldFail = true;

        auto r = mock.initialize();
        QVERIFY(r.isErr());
        QCOMPARE(mock.state(), LifecycleState::Failed);
        QCOMPARE(mock.initializeCount, 1);
        QCOMPARE(mock.startCount, 0);  // 不应该调用 start
    }

    // 3. start 失败 → 状态为 Failed
    void testStartFailure() {
        MockLifecycle mock;
        mock.startShouldFail = true;

        QVERIFY(mock.initialize().isOk());
        QCOMPARE(mock.state(), LifecycleState::Initialized);

        auto r = mock.start();
        QVERIFY(r.isErr());
        QCOMPARE(mock.state(), LifecycleState::Failed);
        QCOMPARE(mock.startCount, 1);
    }

    // 4. stop 幂等性
    void testStopIdempotent() {
        MockLifecycle mock;
        QVERIFY(mock.initialize().isOk());
        QVERIFY(mock.start().isOk());

        mock.stop();
        QCOMPARE(mock.stopCount, 1);
        mock.stop();  // 第二次
        QCOMPARE(mock.stopCount, 2);  // 允许重复调用
    }

    // 5. shutdown 幂等性
    void testShutdownIdempotent() {
        MockLifecycle mock;
        QVERIFY(mock.initialize().isOk());

        mock.shutdown();
        QCOMPARE(mock.shutdownCount, 1);
        mock.shutdown();  // 第二次
        QCOMPARE(mock.shutdownCount, 2);  // 允许重复调用
    }

    // 6. 跳过 start 直接 shutdown
    void testShutdownWithoutStart() {
        MockLifecycle mock;
        QVERIFY(mock.initialize().isOk());

        mock.shutdown();  // 跳过 start
        QCOMPARE(mock.state(), LifecycleState::Shutdown);
        QCOMPARE(mock.startCount, 0);
        QCOMPARE(mock.stopCount, 0);
        QCOMPARE(mock.shutdownCount, 1);
    }

    // 7. 跳过 initialize 直接 shutdown (边界情况)
    void testShutdownWithoutInitialize() {
        MockLifecycle mock;
        // 从未 initialize
        mock.shutdown();
        QCOMPARE(mock.shutdownCount, 1);
        QCOMPARE(mock.initializeCount, 0);
    }

    // 8. isRunning() / isInitialized() 辅助方法
    void testStateHelpers() {
        MockLifecycle mock;
        QVERIFY(!mock.isRunning());
        QVERIFY(!mock.isInitialized());

        QVERIFY(mock.initialize().isOk());
        QVERIFY(!mock.isRunning());
        QVERIFY(mock.isInitialized());

        QVERIFY(mock.start().isOk());
        QVERIFY(mock.isRunning());
        QVERIFY(mock.isInitialized());

        mock.stop();
        QVERIFY(!mock.isRunning());
        QVERIFY(mock.isInitialized());  // Stopped 仍算 initialized

        mock.shutdown();
        QVERIFY(!mock.isRunning());
        QVERIFY(!mock.isInitialized());
    }

    // 9. Error 传播: initialize 失败返回 Error
    void testInitializeErrorContent() {
        MockLifecycle mock;
        mock.initializeShouldFail = true;

        auto r = mock.initialize();
        QVERIFY(r.isErr());
        QCOMPARE(r.unwrapErr().code(), ErrorCode::InternalError);
        QVERIFY(r.unwrapErr().message().contains("Mock initialize failure"));
    }

    // 10. Error 传播: start 失败返回 Error
    void testStartErrorContent() {
        MockLifecycle mock;
        mock.startShouldFail = true;

        QVERIFY(mock.initialize().isOk());
        auto r = mock.start();
        QVERIFY(r.isErr());
        QCOMPARE(r.unwrapErr().code(), ErrorCode::InternalError);
    }
};

QTEST_MAIN(TestLifecycleBasic)
#include "test_lifecycle_basic.moc"
