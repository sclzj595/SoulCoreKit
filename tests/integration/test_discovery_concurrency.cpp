// ============================================================================
// test_discovery_concurrency.cpp — ServiceDiscovery 并发测试 [v2.9.3]
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include <vector>
#include "soul/rpc/service_discovery.h"

using namespace sc::rpc;

class TestDiscoveryConcurrency : public QObject {
    Q_OBJECT

private slots:
    // 1. 并发 register
    void testConcurrentRegister() {
        InMemoryServiceDiscovery discovery;
        discovery.connect(DiscoveryConfig{});

        std::atomic<int> okCount{0};
        const int numThreads = 8;
        const int perThread = 50;

        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&discovery, &okCount, t, perThread]() {
                for (int i = 0; i < perThread; ++i) {
                    ServiceInstance inst{
                        "svc",
                        QString("inst-%1-%2").arg(t).arg(i),
                        "127.0.0.1",
                        8000 + t * 100 + i,
                        0,
                        ServiceInstanceStatus::Unknown,
                        {},
                        {}
                    };
                    if (discovery.registerInstance(inst).isOk()) {
                        okCount.fetch_add(1);
                    }
                }
            });
        }

        for (auto& th : threads) th.join();

        QCOMPARE(okCount.load(), numThreads * perThread);

        auto result = discovery.getInstances("svc");
        QVERIFY(result.isOk());
        QCOMPARE(result.unwrap().size(), numThreads * perThread);
    }

    // 2. 并发 register + getInstances
    void testConcurrentRegisterAndQuery() {
        InMemoryServiceDiscovery discovery;
        discovery.connect(DiscoveryConfig{});

        std::atomic<bool> stop{false};
        std::atomic<int> queries{0};

        std::thread querier([&discovery, &stop, &queries]() {
            while (!stop.load(std::memory_order_acquire)) {
                discovery.getInstances("svc");
                queries.fetch_add(1);
            }
        });

        for (int i = 0; i < 200; ++i) {
            ServiceInstance inst{"svc", QString("i-%1").arg(i), "h", i, 0, ServiceInstanceStatus::Unknown, {}, {}};
            discovery.registerInstance(inst);
        }

        stop.store(true, std::memory_order_release);
        querier.join();

        QVERIFY(queries.load() > 0);
    }

    // 3. 并发 register + unregister
    void testConcurrentRegisterAndUnregister() {
        InMemoryServiceDiscovery discovery;
        discovery.connect(DiscoveryConfig{});

        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&discovery, t]() {
                for (int i = 0; i < 100; ++i) {
                    ServiceInstance inst{"svc", QString("r-%1-%2").arg(t).arg(i), "h", i, 0, ServiceInstanceStatus::Unknown, {}, {}};
                    discovery.registerInstance(inst);
                    discovery.unregisterInstance("svc", "h", i);
                }
            });
        }

        for (auto& th : threads) th.join();
        // 不崩溃即通过
        QVERIFY(true);
    }

    // 4. 并发 getAllServices [v2.9.3]
    void testConcurrentGetAllServices() {
        InMemoryServiceDiscovery discovery;
        discovery.connect(DiscoveryConfig{});

        for (int i = 0; i < 10; ++i) {
            discovery.registerInstance(
                {QString("svc-%1").arg(i), "id", "h", i, 0, ServiceInstanceStatus::Unknown, {}, {}});
        }

        std::atomic<int> ops{0};
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&discovery, &ops]() {
                for (int i = 0; i < 200; ++i) {
                    auto r = discovery.getAllServices();
                    if (r.isOk()) ops.fetch_add(1);
                }
            });
        }

        for (auto& th : threads) th.join();
        QCOMPARE(ops.load(), 4 * 200);
    }
};

QTEST_MAIN(TestDiscoveryConcurrency)
#include "test_discovery_concurrency.moc"
