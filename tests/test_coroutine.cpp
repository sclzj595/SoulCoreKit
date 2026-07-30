#ifdef SOUL_ENABLE_CXX20

#include <QTest>
#include <QCoreApplication>

#include "soul/async/coroutine.h"

using namespace sc;

class TestCoroutine : public QObject {
    Q_OBJECT
private slots:
    void testTaskReturnValue() {
        auto task = []() -> Task<int> {
            co_return 42;
        };
        auto t = task();
        int result = t.get();
        QCOMPARE(result, 42);
    }

    void testTaskAwait() {
        auto inner = []() -> Task<int> {
            co_return 10;
        };
        auto outer = [&]() -> Task<int> {
            int val = co_await inner();
            co_return val + 32;
        };
        auto t = outer();
        int result = t.get();
        QCOMPARE(result, 42);
    }

    void testTaskVoid() {
        bool executed = false;
        auto task = [&]() -> Task<void> {
            executed = true;
            co_return;
        };
        auto t = task();
        t.get();
        QVERIFY(executed);
    }

    void testGenerator() {
        auto gen = []() -> Generator<int> {
            for (int i = 0; i < 5; ++i) {
                co_yield i;
            }
        };
        auto g = gen();
        int sum = 0;
        int count = 0;
        for (int val : g) {
            sum += val;
            count++;
        }
        QCOMPARE(count, 5);
        QCOMPARE(sum, 10);
    }

    void testTaskException() {
        auto task = []() -> Task<int> {
            throw std::runtime_error("test error");
            co_return 0;
        };
        auto t = task();
        bool caught = false;
        try {
            t.get();
        } catch (const std::runtime_error& e) {
            caught = true;
            QCOMPARE(QString(e.what()), QString("test error"));
        }
        QVERIFY(caught);
    }

    void testGeneratorEmpty() {
        auto gen = []() -> Generator<int> {
            co_return;
        };
        auto g = gen();
        int count = 0;
        for (int val : g) {
            Q_UNUSED(val);
            count++;
        }
        QCOMPARE(count, 0);
    }
};

QTEST_GUILESS_MAIN(TestCoroutine)
#include "test_coroutine.moc"

#endif // SOUL_ENABLE_CXX20