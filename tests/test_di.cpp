#include <QTest>
#include <atomic>
#include <thread>

#include "soul/core/singleton.h"
#include "soul/di/container.h"
#include "soul/di/module.h"

class TestService {
public:
    virtual ~TestService() = default;
    virtual int getValue() const { return 0; }
};

class ConcreteService : public TestService {
public:
    int getValue() const override { return 42; }
};

class StatefulService {
public:
    StatefulService()
        : m_counter(0)
    {
    }
    int increment() { return ++m_counter; }
    int getCounter() const { return m_counter; }
private:
    int m_counter;
};

class GlobalSingleton : public sc::Singleton<GlobalSingleton> {
    friend class sc::Singleton<GlobalSingleton>;
public:
    int value() const { return m_value; }
    void setValue(int v) { m_value = v; }
private:
    GlobalSingleton()
        : m_value(100)
    {
    }
    int m_value;
};

class TestDI : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testBindAndResolveTransient();
    void testBindAndResolveSingleton();
    void testBindInstance();
    void testIsRegistered();
    void testUnregister();
    void testClear();
    void testRegistrationCount();

    void testThreadSafety();
    void testSingletonWrapper();
    void testSingletonIntegration();
};

void TestDI::initTestCase()
{
    sc::di::Module::initialize();
}

void TestDI::cleanupTestCase()
{
    sc::di::Module::shutdown();
}

void TestDI::testBindAndResolveTransient()
{
    auto& container = sc::di::Container::instance();
    container.bind<TestService>([]() { return new ConcreteService(); }, sc::di::Lifetime::Transient);

    auto instance1 = container.resolve<TestService>();
    auto instance2 = container.resolve<TestService>();

    QVERIFY(instance1.isOk());
    QVERIFY(instance2.isOk());
    QVERIFY(instance1.unwrap() != instance2.unwrap());
    QCOMPARE(instance1.unwrap()->getValue(), 42);
    QCOMPARE(instance2.unwrap()->getValue(), 42);
}

void TestDI::testBindAndResolveSingleton()
{
    auto& container = sc::di::Container::instance();
    container.bindSingleton<StatefulService>([]() { return new StatefulService(); });

    auto instance1 = container.resolve<StatefulService>();
    auto instance2 = container.resolve<StatefulService>();

    QVERIFY(instance1.isOk());
    QVERIFY(instance2.isOk());
    QVERIFY(instance1.unwrap() == instance2.unwrap());

    int val1 = instance1.unwrap()->increment();
    int val2 = instance2.unwrap()->increment();

    QCOMPARE(val1, 1);
    QCOMPARE(val2, 2);
}

void TestDI::testBindInstance()
{
    auto& container = sc::di::Container::instance();
    auto* external = new StatefulService();

    container.bindInstance(external);

    auto resolved = container.resolve<StatefulService>();
    QVERIFY(resolved.isOk());
    QCOMPARE(resolved.unwrap().get(), external);
}

void TestDI::testIsRegistered()
{
    auto& container = sc::di::Container::instance();

    container.clear();
    QVERIFY(!container.isRegistered<TestService>());

    container.bind<TestService>([]() { return new ConcreteService(); });
    QVERIFY(container.isRegistered<TestService>());
}

void TestDI::testUnregister()
{
    auto& container = sc::di::Container::instance();
    container.bind<TestService>([]() { return new ConcreteService(); });

    QVERIFY(container.isRegistered<TestService>());

    container.unregister<TestService>();
    QVERIFY(!container.isRegistered<TestService>());

    auto instance = container.resolve<TestService>();
    QVERIFY(instance.isErr());
}

void TestDI::testClear()
{
    auto& container = sc::di::Container::instance();
    container.bind<TestService>([]() { return new ConcreteService(); });
    container.bindSingleton<StatefulService>([]() { return new StatefulService(); });

    QCOMPARE(container.registrationCount(), 2);

    container.clear();
    QCOMPARE(container.registrationCount(), 0);

    QVERIFY(!container.isRegistered<TestService>());
    QVERIFY(!container.isRegistered<StatefulService>());
}

void TestDI::testRegistrationCount()
{
    auto& container = sc::di::Container::instance();
    container.clear();

    QCOMPARE(container.registrationCount(), 0);

    container.bind<TestService>([]() { return new ConcreteService(); });
    QCOMPARE(container.registrationCount(), 1);

    container.bindSingleton<StatefulService>([]() { return new StatefulService(); });
    QCOMPARE(container.registrationCount(), 2);

    container.unregister<TestService>();
    QCOMPARE(container.registrationCount(), 1);
}

void TestDI::testThreadSafety()
{
    auto& container = sc::di::Container::instance();
    container.bindSingleton<StatefulService>([]() { return new StatefulService(); });

    const int threadCount = 10;
    const int iterations = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&container, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                auto instance = container.resolve<StatefulService>();
                if (instance.isOk()) {
                    instance.unwrap()->increment();
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto finalInstance = container.resolve<StatefulService>();
    QVERIFY(finalInstance.isOk());
    QCOMPARE(finalInstance.unwrap()->getCounter(), threadCount * iterations);
}

void TestDI::testSingletonWrapper()
{
    auto& container = sc::di::Container::instance();
    container.bindSingleton<StatefulService>([]() { return new StatefulService(); });

    auto wrapped = sc::di::SingletonWrapper<StatefulService>::get();
    auto resolved = container.resolve<StatefulService>();

    QVERIFY(wrapped.isOk());
    QVERIFY(resolved.isOk());
    QVERIFY(wrapped.unwrap() == resolved.unwrap());
}

void TestDI::testSingletonIntegration()
{
    auto& container = sc::di::Container::instance();

    sc::di::registerSingleton<GlobalSingleton>();

    auto resolved = container.resolve<GlobalSingleton>();
    QVERIFY(resolved.isOk());

    QCOMPARE(resolved.unwrap()->value(), 100);

    GlobalSingleton::instance().setValue(200);
    QCOMPARE(resolved.unwrap()->value(), 200);
}

QTEST_APPLESS_MAIN(TestDI)

#include "test_di.moc"
