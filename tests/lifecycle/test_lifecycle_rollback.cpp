// ============================================================================
// test_lifecycle_rollback.cpp — ILifecycle 失败回滚测试 [v2.6.0]
// ============================================================================
// 验证:
//   1. 批量 initialize 中某个失败 → 逆序 shutdown 已成功的
//   2. 批量 start 中某个失败 → 逆序 stop + shutdown 已成功的
//   3. 回滚顺序: 逆序 (后注册先清理)
// ============================================================================

#include <QtTest>
#include <QVector>
#include "soul/core/lifecycle.h"
#include "soul/core/result.h"

using namespace sc;

// --- 记录回滚顺序的 Mock ---

class OrderTrackingLifecycle : public ILifecycle {
public:
    QString name;
    QVector<QString>* log;
    bool initFail = false;
    bool startFail = false;

    OrderTrackingLifecycle(const QString& n, QVector<QString>* l)
        : name(n), log(l) {}

    Result<void> initialize() override {
        log->append(name + ":init");
        if (initFail) {
            return Error(ErrorCode::InternalError, name + " init failed");
        }
        return {};
    }

    Result<void> start() override {
        log->append(name + ":start");
        if (startFail) {
            return Error(ErrorCode::InternalError, name + " start failed");
        }
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

// --- 测试类 ---

class TestLifecycleRollback : public QObject {
    Q_OBJECT

private slots:
    // 1. 全部成功 → 全部初始化并启动
    void testAllSuccess() {
        QVector<QString> log;
        OrderTrackingLifecycle a("A", &log);
        OrderTrackingLifecycle b("B", &log);
        OrderTrackingLifecycle c("C", &log);

        QVERIFY(a.initialize().isOk());
        QVERIFY(b.initialize().isOk());
        QVERIFY(c.initialize().isOk());

        QVERIFY(a.start().isOk());
        QVERIFY(b.start().isOk());
        QVERIFY(c.start().isOk());

        // 正常停止
        c.stop(); b.stop(); a.stop();
        c.shutdown(); b.shutdown(); a.shutdown();

        QCOMPARE(log.size(), 12);  // 3*init + 3*start + 3*stop + 3*shutdown
        QCOMPARE(log[0], QString("A:init"));
        QCOMPARE(log[5], QString("C:start"));
    }

    // 2. 中间 initialize 失败 → 逆序 shutdown 已成功的
    void testInitRollback() {
        QVector<QString> log;
        OrderTrackingLifecycle a("A", &log);
        OrderTrackingLifecycle b("B", &log);
        OrderTrackingLifecycle c("C", &log);
        b.initFail = true;

        // A 成功, B 失败
        QVERIFY(a.initialize().isOk());
        auto r = b.initialize();
        QVERIFY(r.isErr());

        // 逆序回滚: shutdown A (B 从未成功，不需要 shutdown)
        a.shutdown();

        QCOMPARE(log.size(), 3);  // A:init, B:init, A:shutdown
        QCOMPARE(log[0], QString("A:init"));
        QCOMPARE(log[1], QString("B:init"));
        QCOMPARE(log[2], QString("A:shutdown"));  // 逆序: A 被回滚
    }

    // 3. 中间 start 失败 → 逆序 stop + shutdown 已成功的
    void testStartRollback() {
        QVector<QString> log;
        OrderTrackingLifecycle a("A", &log);
        OrderTrackingLifecycle b("B", &log);
        OrderTrackingLifecycle c("C", &log);
        b.startFail = true;

        // 全部 init 成功
        QVERIFY(a.initialize().isOk());
        QVERIFY(b.initialize().isOk());
        QVERIFY(c.initialize().isOk());

        // A start 成功, B start 失败
        QVERIFY(a.start().isOk());
        auto r = b.start();
        QVERIFY(r.isErr());

        // 逆序回滚: stop A, shutdown A
        a.stop();
        a.shutdown();

        QCOMPARE(log.size(), 7);  // A:init, B:init, C:init, A:start, B:start, A:stop, A:shutdown
        QCOMPARE(log[3], QString("A:start"));
        QCOMPARE(log[4], QString("B:start"));
        QCOMPARE(log[5], QString("A:stop"));
        QCOMPARE(log[6], QString("A:shutdown"));
    }

    // 4. 第一个 initialize 就失败 → 无回滚
    void testFirstInitFails() {
        QVector<QString> log;
        OrderTrackingLifecycle a("A", &log);
        OrderTrackingLifecycle b("B", &log);
        a.initFail = true;

        auto r = a.initialize();
        QVERIFY(r.isErr());

        // B 从未初始化，不需要回滚
        QCOMPARE(log.size(), 1);  // 仅 A:init
    }

    // 5. 全部 initialize 成功，第一个 start 失败 → 无已启动的服务需回滚
    void testFirstStartFails() {
        QVector<QString> log;
        OrderTrackingLifecycle a("A", &log);
        OrderTrackingLifecycle b("B", &log);
        a.startFail = true;

        QVERIFY(a.initialize().isOk());
        QVERIFY(b.initialize().isOk());

        auto r = a.start();
        QVERIFY(r.isErr());

        // 无已启动服务需回滚
        // 但已初始化的服务需要 shutdown
        b.shutdown();
        a.shutdown();

        QCOMPARE(log[0], QString("A:init"));
        QCOMPARE(log[1], QString("B:init"));
        QCOMPARE(log[2], QString("A:start"));  // 失败了
    }
};

QTEST_MAIN(TestLifecycleRollback)
#include "test_lifecycle_rollback.moc"
