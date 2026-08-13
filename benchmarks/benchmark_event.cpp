// ============================================================================
// benchmark_event.cpp — EventBus 基准 [v2.8.0]
// ============================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <algorithm>

struct TestEvent { int value; };

class MockEventBus {
    std::mutex m_mutex;
    std::vector<std::function<void(const TestEvent&)>> m_subscribers;
public:
    void subscribe(std::function<void(const TestEvent&)> h) {
        std::lock_guard lock(m_mutex);
        m_subscribers.push_back(std::move(h));
    }

    void publish(const TestEvent& e) {
        std::vector<std::function<void(const TestEvent&)>> snapshot;
        {
            std::lock_guard lock(m_mutex);
            snapshot = m_subscribers;
        }
        for (auto& h : snapshot) h(e);
    }
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== EventBus Benchmark ===\n";

    const int ITER = 50000;

    // --- Single subscriber publish ---
    {
        MockEventBus bus;
        std::atomic<int> received{0};
        bus.subscribe([&received](const TestEvent&) { received++; });

        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            bus.publish(TestEvent{i});
            latencies.push_back(op.nsecsElapsed() / 1000);
        }
        auto total = timer.elapsed();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  Publish (1 subscriber):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    received:    " << received.load() << "\n";

        std::sort(latencies.begin(), latencies.end());
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";
        std::cout << "    p99:         " << latencies[latencies.size() * 99 / 100] << " μs\n";
    }

    // --- 10 subscribers publish ---
    {
        MockEventBus bus;
        std::atomic<int> received{0};
        for (int s = 0; s < 10; ++s) {
            bus.subscribe([&received](const TestEvent&) { received++; });
        }

        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < ITER; ++i) {
            bus.publish(TestEvent{i});
        }
        auto total = timer.elapsed();

        std::cout << "\n  Publish (10 subscribers):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    received:    " << received.load() << " (should be " << ITER * 10 << ")\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
