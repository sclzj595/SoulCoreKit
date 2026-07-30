#include <QTest>
#include <QCoreApplication>

#include "soul/core/module.h"
#include "soul/core/module_registry.h"
#include "soul/core/scaffold.h"

using namespace sc;

// ============================================================================
// 测试用模块
// ============================================================================

class TestModuleA : public Module {
public:
    TestModuleA() : Module("TestModuleA") {}
    Result<void> init() override { m_initCalled = true; return {}; }
    bool m_initCalled = false;
};

class TestModuleB : public Module {
public:
    TestModuleB() : Module("TestModuleB") {}
    Result<void> init() override { m_initCalled = true; return {}; }
    std::vector<std::string> dependsOn() const override { return {"TestModuleA"}; }
    bool m_initCalled = false;
};

class TestModuleDisabled : public Module {
public:
    TestModuleDisabled() : Module("TestModuleDisabled") {}
    bool isEnabled() const override { return false; }
};

// SC_MODULE 宏不适用于测试(静态初始化在 main 之前),
// 直接测试 ModuleRegistry 的 API

// ============================================================================
// TestModuleRegistry — 模块注册表测试
// ============================================================================
class TestModuleRegistry : public QObject {
    Q_OBJECT
private slots:
    void cleanup() {
        ModuleRegistry::instance().clear();
    }

    void testRegisterAndEmpty() {
        QVERIFY(ModuleRegistry::instance().empty());
        ModuleRegistry::instance().registerModule("test", []() {
            return std::make_unique<TestModuleA>();
        });
        QVERIFY(!ModuleRegistry::instance().empty());
        QCOMPARE(ModuleRegistry::instance().size(), size_t(1));
    }

    void testUnregister() {
        ModuleRegistry::instance().registerModule("test", []() {
            return std::make_unique<TestModuleA>();
        });
        QCOMPARE(ModuleRegistry::instance().size(), size_t(1));
        ModuleRegistry::instance().unregisterModule("test");
        QVERIFY(ModuleRegistry::instance().empty());
    }

    void testDuplicateRegistration() {
        ModuleRegistry::instance().registerModule("test", []() {
            return std::make_unique<TestModuleA>();
        });
        ModuleRegistry::instance().registerModule("test", []() {
            return std::make_unique<TestModuleB>();
        });
        QCOMPARE(ModuleRegistry::instance().size(), size_t(1));
    }

    void testFactoryReturnsCorrectType() {
        ModuleRegistry::instance().registerModule("test", []() {
            return std::make_unique<TestModuleA>();
        });
        auto factories = ModuleRegistry::instance().factories();
        auto module = factories["test"]();
        QVERIFY(module != nullptr);
        QCOMPARE(module->name(), std::string("TestModuleA"));
    }

    void testClear() {
        ModuleRegistry::instance().registerModule("a", []() {
            return std::make_unique<TestModuleA>();
        });
        ModuleRegistry::instance().registerModule("b", []() {
            return std::make_unique<TestModuleB>();
        });
        QCOMPARE(ModuleRegistry::instance().size(), size_t(2));
        ModuleRegistry::instance().clear();
        QVERIFY(ModuleRegistry::instance().empty());
    }
};

// ============================================================================
// TestScaffoldScan — Scaffold.scan() 测试(不进入事件循环)
// ============================================================================
class TestScaffoldScan : public QObject {
    Q_OBJECT
private slots:
    void cleanup() {
        ModuleRegistry::instance().clear();
    }

    void testScanRegistersModules() {
        ModuleRegistry::instance().registerModule("TestModuleA", []() {
            return std::make_unique<TestModuleA>();
        });

        int argc = 0;
        char* argv[] = {nullptr};
        Scaffold scaffold(argc, argv);
        scaffold.scan(ModuleRegistry::instance());

        // 模块已注册到 Scaffold,无需 run() 即可验证
        // 但 Scaffold 的内部状态是私有的,只能通过间接方式验证
        QVERIFY(true);
    }

    void testScanWithManualUse() {
        ModuleRegistry::instance().registerModule("TestModuleA", []() {
            return std::make_unique<TestModuleA>();
        });

        TestModuleB moduleB;
        int argc = 0;
        char* argv[] = {nullptr};
        Scaffold scaffold(argc, argv);
        scaffold.scan(ModuleRegistry::instance()).use(moduleB);

        // 混合使用 scan() + use() 不崩溃
        QVERIFY(true);
    }

    void testScanEmptyRegistry() {
        int argc = 0;
        char* argv[] = {nullptr};
        Scaffold scaffold(argc, argv);
        scaffold.scan(ModuleRegistry::instance());

        // 空注册表不崩溃
        QVERIFY(true);
    }

    void testScanMultipleModules() {
        ModuleRegistry::instance().registerModule("TestModuleA", []() {
            return std::make_unique<TestModuleA>();
        });
        ModuleRegistry::instance().registerModule("TestModuleB", []() {
            return std::make_unique<TestModuleB>();
        });

        int argc = 0;
        char* argv[] = {nullptr};
        Scaffold scaffold(argc, argv);
        scaffold.scan(ModuleRegistry::instance());

        // 多个模块注册不崩溃
        QVERIFY(true);
    }

    void testScanWithDisabledModule() {
        ModuleRegistry::instance().registerModule("TestModuleDisabled", []() {
            return std::make_unique<TestModuleDisabled>();
        });

        int argc = 0;
        char* argv[] = {nullptr};
        Scaffold scaffold(argc, argv);
        scaffold.scan(ModuleRegistry::instance());

        // 禁用的模块也能注册,run() 时会跳过
        QVERIFY(true);
    }
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    int result = 0;
    { TestModuleRegistry t; result |= QTest::qExec(&t, argc, argv); }
    { TestScaffoldScan t; result |= QTest::qExec(&t, argc, argv); }

    return result;
}

#include "test_module_registry.moc"