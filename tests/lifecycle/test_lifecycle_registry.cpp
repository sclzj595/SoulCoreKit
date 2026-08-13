// ============================================================================
// test_lifecycle_registry.cpp — ServiceRegistry 生命周期集成测试 [v2.6.0]
// ============================================================================
// 验证:
//   1. ServiceRegistry 四阶段批量管理
//   2. initializeAll 失败自动回滚
//   3. startAll 失败自动 stop + shutdown
//   4. 逆序关闭保证
// ============================================================================

#include <QtTest>
#include <QVector>
#include "soul/core/lifecycle.h"
#include "soul/core/result.h"

using namespace sc;

// --- 模拟 ServiceRegistry 的核心逻辑 ---
// 此处测试 ServiceRegistry 的回滚逻辑，不直接依赖 application 模块

class TestService : public ILifecycle {
public:
    QString name;
    QVector<QString>* log;
    bool initFail = false;
    bool startFail = false;

    TestService(const QString& n, QVector<QString>* l) : name(n), log(l) {}

    Result<void> initialize() override {
        log->append(name + ":init");
        if (initFail) return Error(ErrorCode::InternalError, name + " init failed");
        return {};
    }

    Result<void> start() override {
        log->append(name + ":start");
        if (startFail) return Error(ErrorCode::InternalError, name + " start failed");
        return {};
    }

    void stop() noexcept override {
        log->append(name + ":stop");
    }

    void shutdown() noexcept override {
        log->append(name + ":shutdown");
    }

    LifecycleState state() const override {
        return LifecycleState::Constructed;
    }
};

// 模拟 ServiceRegistry 的批量操作逻辑
struct MockRegistry {
    std::vector<ILifecycle*> services;

    Result<void> initializeAll() {
        for (size_t i = 0; i < services.size(); ++i) {
            auto r = services[i]->initialize();
            if (r.isErr()) {
                // 逆序回滚
                for (size_t j = i; j > 0; --j) {
                    services[j - 1]->shutdown();
                }
                return r;
            }
        }
        return {};
    }

    Result<void> startAll() {
        for (size_t i = 0; i < services.size(); ++i) {
            auto r = services[i]->start();
            if (r.isErr()) {
                // 逆序 stop + shutdown
                for (size_t j = i; j > 0; --j) {
                    services[j - 1]->stop();
                }
                for (size_t j = i; j > 0; --j) {
                    services[j - 1]->shutdown();
                }
                return r;
            }
        }
        return {};
    }

    void stopAll() {
        for (auto it = services.rbegin(); it != services.rend(); ++it) {
            (*it)->stop();
        }
    }

    void shutdownAll() {
        for (auto it = services.rbegin(); it != services.rend(); ++it) {
            (*it)->shutdown();
        }
    }
};

// --- 测试类 ---

class TestLifecycleRegistry : public QObject {
    Q_OBJECT

private slots:
    // 1. 全流程成功
    void testFullFlow() {
        QVector<QString> log;
        TestService a("A", &log);
        TestService b("B", &log);
        MockRegistry reg;
        reg.services = {&a, &b};

        QVERIFY(reg.initializeAll().isOk());
        QVERIFY(reg.startAll().isOk());
        reg.stopAll();
        reg.shutdownAll();

        // 验证调用顺序
        QCOMPARE(log[0], QString("A:init"));
        QCOMPARE(log[1], QString("B:init"));
        QCOMPARE(log[2], QString("A:start"));
        QCOMPARE(log[3], QString("B:start"));
        // 逆序 stop
        QCOMPARE(log[4], QString("B:stop"));
        QCOMPARE(log[5], QString("A:stop"));
        // 逆序 shutdown
        QCOMPARE(log[6], QString("B:shutdown"));
        QCOMPARE(log[7], QString("A:shutdown"));
    }

    // 2. initializeAll 中间失败回滚
    void testInitRollback() {
        QVector<QString> log;
        TestService a("A", &log);
        TestService b("B", &log);
        TestService c("C", &log);
        b.initFail = true;
        MockRegistry reg;
        reg.services = {&a, &b, &c};

        auto r = reg.initializeAll();
        QVERIFY(r.isErr());

        // A 被初始化然后被回滚 shutdown
        QCOMPARE(log[0], QString("A:init"));
        QCOMPARE(log[1], QString("B:init"));   // 失败了
        QCOMPARE(log[2], QString("A:shutdown")); // A 被回滚
        QCOMPARE(log.size(), 3);                // C 从未初始化
    }

    // 3. startAll 中间失败回滚
    void testStartRollback() {
        QVector<QString> log;
        TestService a("A", &log);
        TestService b("B", &log);
        TestService c("C", &log);
        b.startFail = true;
        MockRegistry reg;
        reg.services = {&a, &b, &c};

        QVERIFY(reg.initializeAll().isOk());
        auto r = reg.startAll();
        QVERIFY(r.isErr());

        // v3.0.0: 修正日志索引 — initializeAll 已写入 3 条 (A/B/C:init, log[0..2]),
        // start 阶段日志从 log[3] 开始。
        QCOMPARE(log[0], QString("A:init"));
        QCOMPARE(log[1], QString("B:init"));
        QCOMPARE(log[2], QString("C:init"));
        QCOMPARE(log[3], QString("A:start"));
        QCOMPARE(log[4], QString("B:start"));   // 失败了
        QCOMPARE(log[5], QString("A:stop"));    // A 被 stop
        QCOMPARE(log[6], QString("A:shutdown")); // A 被 shutdown
        // C 从未 start
    }

    // 4. 空注册表
    void testEmptyRegistry() {
        MockRegistry reg;
        QVERIFY(reg.initializeAll().isOk());
        QVERIFY(reg.startAll().isOk());
        reg.stopAll();    // 不应崩溃
        reg.shutdownAll(); // 不应崩溃
    }

    // 5. 单个服务完整流程
    void testSingleService() {
        QVector<QString> log;
        TestService s("Single", &log);
        MockRegistry reg;
        reg.services = {&s};

        QVERIFY(reg.initializeAll().isOk());
        QVERIFY(reg.startAll().isOk());
        reg.stopAll();
        reg.shutdownAll();

        QCOMPARE(log.size(), 4);
        QCOMPARE(log[0], QString("Single:init"));
        QCOMPARE(log[3], QString("Single:shutdown"));
    }
};

QTEST_MAIN(TestLifecycleRegistry)
#include "test_lifecycle_registry.moc"
