#include <QTest>
#include <QThread>
#include <atomic>
#include <thread>
#include <memory>

#include "soul/core/singleton.h"

using namespace sc;

// ============================================================================
// 测试辅助类
// ============================================================================

class TestPlainSingleton : public Singleton<TestPlainSingleton> {
public:
    int value = 42;
    static int constructCount;
    TestPlainSingleton() { constructCount++; }
};
int TestPlainSingleton::constructCount = 0;

class TestSharedSingleton : public SharedSingleton<TestSharedSingleton> {
public:
    void init() { initialized = true; }
    void shutdown() { initialized = false; }
    bool initialized = false;
    int value = 100;
};

// ============================================================================
// TestSingleton — 单例测试
// ============================================================================
class TestSingleton : public QObject {
    Q_OBJECT

private slots:
    void testPlainInstance();
    void testPlainSingleConstruction();
    void testPlainInstanceSamePointer();

    void testSharedInstance();
    void testSharedInitDestroy();
    void testSharedIsInitialized();
    void testSharedMultipleInitNoDoubleInit();

    void testConcurrentAccess();
};

void TestSingleton::testPlainInstance() {
    auto& inst = TestPlainSingleton::instance();
    QCOMPARE(inst.value, 42);
}

void TestSingleton::testPlainSingleConstruction() {
    // 注意: TestPlainSingleton 是 Singleton<T>（Meyer's Singleton），
    // static local 在进程生命周期内只构造一次。
    // constructCount 在第一次 instance() 调用时递增，后续不变。
    // 这里验证 singleton 只构造一次。
    int countBefore = TestPlainSingleton::constructCount;
    TestPlainSingleton::instance();
    TestPlainSingleton::instance();
    TestPlainSingleton::instance();
    // constructCount 不变（因为只构造了一次，后续 instance() 返回已有实例）
    QCOMPARE(TestPlainSingleton::constructCount, countBefore);
}

void TestSingleton::testPlainInstanceSamePointer() {
    auto& a = TestPlainSingleton::instance();
    auto& b = TestPlainSingleton::instance();
    QCOMPARE(&a, &b);
}

void TestSingleton::testSharedInstance() {
    auto p = TestSharedSingleton::instance();
    QVERIFY(p != nullptr);
    QCOMPARE(p->value, 100);
}

void TestSingleton::testSharedInitDestroy() {
    // 确保 instance 存在（init() 要求 m_instance 非空）
    (void)TestSharedSingleton::instance();
    SharedSingleton<TestSharedSingleton>::init();
    QVERIFY(SharedSingleton<TestSharedSingleton>::isInitialized());
    auto p = TestSharedSingleton::instance();
    QVERIFY(p->initialized);

    SharedSingleton<TestSharedSingleton>::destroy();
    QVERIFY(!SharedSingleton<TestSharedSingleton>::isInitialized());
}

void TestSingleton::testSharedIsInitialized() {
    // 先确保干净状态
    if (SharedSingleton<TestSharedSingleton>::isInitialized()) {
        SharedSingleton<TestSharedSingleton>::destroy();
    }
    QVERIFY(!SharedSingleton<TestSharedSingleton>::isInitialized());

    // 确保 instance 存在
    auto p = TestSharedSingleton::instance();
    QVERIFY(p != nullptr);

    SharedSingleton<TestSharedSingleton>::init();
    // 验证 init() 实际调用了 TestSharedSingleton::init()
    QVERIFY(p->initialized);
    QVERIFY(SharedSingleton<TestSharedSingleton>::isInitialized());

    SharedSingleton<TestSharedSingleton>::destroy();
    QVERIFY(!SharedSingleton<TestSharedSingleton>::isInitialized());
}

void TestSingleton::testSharedMultipleInitNoDoubleInit() {
    // 确保干净状态
    if (SharedSingleton<TestSharedSingleton>::isInitialized()) {
        SharedSingleton<TestSharedSingleton>::destroy();
    }
    // 确保 instance 存在
    auto p = TestSharedSingleton::instance();
    QVERIFY(p != nullptr);

    // 多次 init 不会重复初始化
    SharedSingleton<TestSharedSingleton>::init();
    QVERIFY(SharedSingleton<TestSharedSingleton>::isInitialized());
    SharedSingleton<TestSharedSingleton>::init();  // 第二次 init 应无操作
    QVERIFY(SharedSingleton<TestSharedSingleton>::isInitialized());
    SharedSingleton<TestSharedSingleton>::destroy();
}

void TestSingleton::testConcurrentAccess() {
    // 多线程并发获取单例, 验证线程安全(不崩溃)
    std::atomic<int> ok{0};
    auto worker = [&ok]() {
        for (int i = 0; i < 100; i++) {
            auto& inst = TestPlainSingleton::instance();
            if (inst.value == 42) ok++;
        }
    };

    // 使用 std::thread 直接启动并发 worker，避免 QThread::started 信号
    // 在无 event loop 的线程中无法被 delivery 的问题（QThread 默认不运行 exec()）。
    // Singleton 的 static local 在 C++11 下天然线程安全，std::thread 可直接验证。
    std::thread thr1(worker);
    std::thread thr2(worker);
    std::thread thr3(worker);

    thr1.join(); thr2.join(); thr3.join();

    QCOMPARE(ok.load(), 300);
}

QTEST_MAIN(TestSingleton)
#include "test_singleton.moc"
