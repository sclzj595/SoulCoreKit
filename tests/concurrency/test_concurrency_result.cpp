// ============================================================================
// test_concurrency_result.cpp — Result<T> 并发安全测试 [v2.6.0]
// ============================================================================
// 验证:
//   1. Result<T> 值语义 → 多线程操作独立副本安全
//   2. Error 不可变性 → 多线程读取无竞争
//   3. Result<void> 特化并发安全
//
// 运行建议: cmake -DENABLE_TSAN=ON && ctest -L concurrency -R test_concurrency_result
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include <vector>
#include "soul/core/result.h"
#include "soul/core/error.h"

using namespace sc;

class TestConcurrencyResult : public QObject {
    Q_OBJECT

private slots:
    // 1. 多线程各自创建和使用 Result (值语义，天然安全)
    void testValueSemanticsThreadSafe() {
        std::vector<std::thread> threads;
        std::atomic<int> successCount{0};
        const int numThreads = 8;
        const int iterations = 500;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&successCount, iterations]() {
                for (int i = 0; i < iterations; ++i) {
                    // 每个线程创建自己的 Result
                    auto r = Result<int>::ok(i);
                    QVERIFY(r.isOk());
                    QCOMPARE(r.unwrap(), i);

                    auto r2 = r.map([](int v) { return v * 2; });
                    QVERIFY(r2.isOk());
                    QCOMPARE(r2.unwrap(), i * 2);

                    successCount.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        for (auto& t : threads) t.join();
        QCOMPARE(successCount.load(), numThreads * iterations);
    }

    // 2. 多线程共享读取同一个 Error (不可变对象)
    void testSharedErrorRead() {
        auto sharedError = std::make_shared<Error>(
            ErrorCode::Timeout, "Shared timeout error"
        );

        std::vector<std::thread> threads;
        std::atomic<int> readsDone{0};
        const int numThreads = 16;
        const int readsPerThread = 200;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&sharedError, &readsDone, readsPerThread]() {
                for (int i = 0; i < readsPerThread; ++i) {
                    QCOMPARE(sharedError->code(), ErrorCode::Timeout);
                    QVERIFY(sharedError->message().contains("timeout", Qt::CaseInsensitive));
                    QVERIFY(sharedError->isNetworkError());
                    readsDone.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        for (auto& t : threads) t.join();
        QCOMPARE(readsDone.load(), numThreads * readsPerThread);
    }

    // 3. 多线程各自 map/andThen (独立副本)
    void testConcurrentMonadicOps() {
        std::vector<std::thread> threads;
        std::atomic<int> opsDone{0};
        const int numThreads = 8;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&opsDone, t]() {
                for (int i = 0; i < 300; ++i) {
                    auto r = Result<int>::ok(t * 1000 + i);

                    auto mapped = r.map([](int v) { return v * 3; });
                    QCOMPARE(mapped.unwrap(), (t * 1000 + i) * 3);

                    auto chained = r.andThen([](int v) -> Result<QString> {
                        return Result<QString>::ok(QString::number(v));
                    });
                    QVERIFY(chained.isOk());

                    opsDone.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        for (auto& t : threads) t.join();
        QCOMPARE(opsDone.load(), numThreads * 300);
    }

    // 4. Result<void> 并发安全
    void testResultVoidConcurrent() {
        std::atomic<int> okCount{0};
        std::atomic<int> errCount{0};

        std::vector<std::thread> threads;
        const int numThreads = 8;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&okCount, &errCount, t]() {
                for (int i = 0; i < 200; ++i) {
                    if (i % 2 == 0) {
                        auto r = Result<void>::ok();
                        QVERIFY(r.isOk());
                        okCount.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        auto r = Result<void>::err(
                            Error(ErrorCode::Unknown, QString("thread %1 err").arg(t))
                        );
                        QVERIFY(r.isErr());
                        errCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }

        for (auto& t : threads) t.join();
        int expectedEach = numThreads * 100;  // 200/2 = 100 per thread
        QCOMPARE(okCount.load(), expectedEach);
        QCOMPARE(errCount.load(), expectedEach);
    }

    // 5. Error withContext 并发读取 (不可变)
    void testErrorContextConcurrentRead() {
        auto err = std::make_shared<Error>(
            Error(ErrorCode::NetworkError, "Connection lost")
                .withContext("request_id", QString("req-999"))
                .withContext("retry", 3)
        );

        std::vector<std::thread> threads;
        std::atomic<int> readsDone{0};
        const int numThreads = 8;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&err, &readsDone]() {
                for (int i = 0; i < 200; ++i) {
                    QCOMPARE(err->contextValue("request_id").toString(), QString("req-999"));
                    QCOMPARE(err->contextValue("retry").toInt(), 3);
                    QVERIFY(err->isNetworkError());
                    readsDone.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        for (auto& t : threads) t.join();
        QCOMPARE(readsDone.load(), numThreads * 200);
    }
};

QTEST_MAIN(TestConcurrencyResult)
#include "test_concurrency_result.moc"
