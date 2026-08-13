// ============================================================================
// test_lifecycle_module.cpp — Module ILifecycle 集成测试 [v2.6.0]
// ============================================================================
// 验证:
//   1. Module 正确实现 ILifecycle 接口
//   2. Module 状态机自动转换
//   3. Module::init/onStart/onStop/cleanup → ILifecycle::initialize/start/stop/shutdown 映射
//   4. Module::dependsOn() + priority() 不变
// ============================================================================

#include <QtTest>
#include "soul/core/module.h"
#include "soul/core/lifecycle.h"

using namespace sc;

// --- 测试用 Module 子类 ---

class LifecycleModule : public Module {
public:
    int initCalls = 0;
    int startCalls = 0;
    int stopCalls = 0;
    int cleanupCalls = 0;
    bool initFail = false;
    bool startFail = false;

    LifecycleModule(const std::string& name) : Module(name) {}

    Result<void> init() override {
        initCalls++;
        if (initFail) return Error(ErrorCode::InternalError, name() + " init failed");
        return {};
    }

    Result<void> onStart() override {
        startCalls++;
        if (startFail) return Error(ErrorCode::InternalError, name() + " start failed");
        return {};
    }

    void onStop() override { stopCalls++; }
    void cleanup() override { cleanupCalls++; }
};

// --- 测试类 ---

class TestLifecycleModule : public QObject {
    Q_OBJECT

private slots:
    // 1. Module 通过 ILifecycle 接口调用
    void testModuleAsILifecycle() {
        LifecycleModule mod("TestMod");
        ILifecycle* lifecycle = &mod;

        QCOMPARE(lifecycle->state(), LifecycleState::Constructed);

        auto r1 = lifecycle->initialize();
        QVERIFY(r1.isOk());
        QCOMPARE(lifecycle->state(), LifecycleState::Initialized);
        QCOMPARE(mod.initCalls, 1);

        auto r2 = lifecycle->start();
        QVERIFY(r2.isOk());
        QCOMPARE(lifecycle->state(), LifecycleState::Running);
        QCOMPARE(mod.startCalls, 1);

        lifecycle->stop();
        QCOMPARE(lifecycle->state(), LifecycleState::Stopped);
        QCOMPARE(mod.stopCalls, 1);

        lifecycle->shutdown();
        QCOMPARE(lifecycle->state(), LifecycleState::Shutdown);
        QCOMPARE(mod.cleanupCalls, 1);
    }

    // 2. Module init 失败 → ILifecycle::initialize 返回 Error
    void testModuleInitFailure() {
        LifecycleModule mod("FailMod");
        mod.initFail = true;

        ILifecycle* lifecycle = &mod;
        auto r = lifecycle->initialize();
        QVERIFY(r.isErr());
        QCOMPARE(lifecycle->state(), LifecycleState::Failed);
        QCOMPARE(mod.initCalls, 1);
    }

    // 3. Module onStart 失败 → ILifecycle::start 返回 Error
    void testModuleStartFailure() {
        LifecycleModule mod("FailMod");
        mod.startFail = true;

        ILifecycle* lifecycle = &mod;
        QVERIFY(lifecycle->initialize().isOk());
        auto r = lifecycle->start();
        QVERIFY(r.isErr());
        QCOMPARE(lifecycle->state(), LifecycleState::Failed);
    }

    // 4. Module 默认方法不崩溃
    void testModuleDefaults() {
        class DefaultModule : public Module {
        public:
            DefaultModule() : Module("Default") {}
        };

        DefaultModule mod;
        ILifecycle* lifecycle = &mod;

        QVERIFY(lifecycle->initialize().isOk());
        QVERIFY(lifecycle->start().isOk());
        lifecycle->stop();
        lifecycle->shutdown();
        QCOMPARE(lifecycle->state(), LifecycleState::Shutdown);
    }

    // 5. Module::dependsOn() 仍然可用
    void testModuleDependsOn() {
        class DepModule : public Module {
        public:
            DepModule() : Module("Dep") {}
            std::vector<std::string> dependsOn() const override {
                return {"Core", "Network"};
            }
        };

        DepModule mod;
        auto deps = mod.dependsOn();
        QCOMPARE(deps.size(), size_t(2));
        QCOMPARE(QString::fromStdString(deps[0]), QString("Core"));
        QCOMPARE(QString::fromStdString(deps[1]), QString("Network"));
    }

    // 6. Module::priority() 仍然可用
    void testModulePriority() {
        class HighPrioModule : public Module {
        public:
            HighPrioModule() : Module("HighPrio") {}
            int priority() const override { return 100; }
        };

        HighPrioModule mod;
        QCOMPARE(mod.priority(), 100);
    }

    // 7. Module::isEnabled() 条件装配
    void testModuleIsEnabled() {
        class ConditionalModule : public Module {
        public:
            ConditionalModule() : Module("Cond") {}
            bool isEnabled() const override { return false; }
        };

        ConditionalModule mod;
        QVERIFY(!mod.isEnabled());
    }

    // 8. Module 默认 isEnabled() 返回 true
    void testModuleDefaultEnabled() {
        LifecycleModule mod("Default");
        QVERIFY(mod.isEnabled());
    }

    // 9. 多个 Module 独立生命周期
    void testMultipleModules() {
        LifecycleModule a("A");
        LifecycleModule b("B");

        QVERIFY(a.initialize().isOk());
        QVERIFY(b.initialize().isOk());
        QVERIFY(a.start().isOk());
        QVERIFY(b.start().isOk());

        QCOMPARE(a.initCalls, 1);
        QCOMPARE(b.initCalls, 1);

        a.stop();
        b.stop();
        a.shutdown();
        b.shutdown();

        QCOMPARE(a.cleanupCalls, 1);
        QCOMPARE(b.cleanupCalls, 1);
    }
};

QTEST_MAIN(TestLifecycleModule)
#include "test_lifecycle_module.moc"
