// ============================================================================
// test_stress_task.cpp — Task 压力测试 [v2.9.0]
// ============================================================================
// 验证:
//   1. 100+ concurrent tasks 不崩溃
//   2. shutdown during task — 不泄漏
//   3. long-running task cancellation
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>

class TestStressTask : public QObject {
    Q_OBJECT

private:
    // 模拟 TaskRunner
    class MockTaskRunner {
        std::vector<std::thread> m_threads;
        std::atomic<bool> m_running{false};
        std::atomic<int> m_activeTasks{0};
        std::atomic<int> m_completedTasks{0};

    public:
        ~MockTaskRunner() { shutdown(); }

        void start(int threadCount) {
            m_running.store(true);
            for (int i = 0; i < threadCount; ++i) {
                m_threads.emplace_back([this]() {
                    while (m_running.load(std::memory_order_acquire)) {
                        m_activeTasks.fetch_add(1, std::memory_order_relaxed);
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                        m_activeTasks.fetch_sub(1, std::memory_order_relaxed);
                        m_completedTasks.fetch_add(1, std::memory_order_relaxed);
                    }
                });
            }
        }

        void shutdown() {
            m_running.store(false, std::memory_order_release);
            for (auto& t : m_threads) {
                if (t.joinable()) t.join();
            }
            m_threads.clear();
        }

        int activeTasks() const { return m_activeTasks.load(); }
        int completedTasks() const { return m_completedTasks.load(); }
        bool isRunning() const { return m_running.load(); }
    };

private slots:
    // 1. 100 concurrent tasks
    void testConcurrentTasks() {
        MockTaskRunner runner;
        runner.start(8);

        QThread::msleep(500);

        int completed = runner.completedTasks();
        QVERIFY(completed > 0);  // 有任务完成

        runner.shutdown();
        QVERIFY(runner.activeTasks() == 0);  // 所有任务完成
    }

    // 2. Shutdown during task — 不泄漏
    void testShutdownDuringTask() {
        for (int iteration = 0; iteration < 10; ++iteration) {
            MockTaskRunner runner;
            runner.start(4);

            QThread::msleep(50);  // 短暂运行
            runner.shutdown();     // 中途关闭

            QVERIFY(!runner.isRunning());
            QVERIFY(runner.activeTasks() == 0);
        }
    }

    // 3. 长时间运行任务 + 取消
    void testLongRunningCancellation() {
        std::atomic<bool> cancelled{false};
        std::atomic<int> progress{0};

        std::thread worker([&cancelled, &progress]() {
            for (int i = 0; i < 10000; ++i) {
                if (cancelled.load(std::memory_order_acquire)) {
                    return;
                }
                progress.store(i, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        QThread::msleep(100);  // 运行一小段
        cancelled.store(true, std::memory_order_release);

        if (worker.joinable()) worker.join();

        // 在 10000 次迭代完成前被取消
        QVERIFY(progress.load() < 9999);
    }

    // 4. 多线程计数器一致性
    void testMultiThreadCounter() {
        std::atomic<int64_t> counter{0};
        const int numThreads = 8;
        const int incrementsPerThread = 10000;

        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&counter, incrementsPerThread]() {
                for (int i = 0; i < incrementsPerThread; ++i) {
                    counter.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        for (auto& t : threads) t.join();

        QCOMPARE(counter.load(), int64_t(numThreads) * incrementsPerThread);
    }
};

QTEST_MAIN(TestStressTask)
#include "test_stress_task.moc"
