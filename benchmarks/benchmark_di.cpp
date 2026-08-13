// ============================================================================
// benchmark_di.cpp — DI Container 基准 [v2.8.0]
// ============================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <memory>
#include <mutex>

// 模拟 DI resolve (Singleton)
class MockService {
public:
    static int constructionCount;
    MockService() { constructionCount++; }
    int value = 42;
};
int MockService::constructionCount = 0;

class MockDIContainer {
    std::mutex m_mutex;
    std::shared_ptr<MockService> m_singleton;
public:
    std::shared_ptr<MockService> resolveSingleton() {
        if (!m_singleton) {
            std::lock_guard lock(m_mutex);
            if (!m_singleton) {
                m_singleton = std::make_shared<MockService>();
            }
        }
        return m_singleton;
    }

    std::shared_ptr<MockService> resolveTransient() {
        return std::make_shared<MockService>();
    }
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== DI Container Benchmark ===\n";

    const int ITER = 100000;

    MockDIContainer container;

    // --- Singleton resolve ---
    {
        MockService::constructionCount = 0;
        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            auto svc = container.resolveSingleton();
            (void)svc->value;
            latencies.push_back(op.nsecsElapsed() / 1000);
        }
        auto total = timer.elapsed();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  Singleton resolve:\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";
        std::cout << "    constructions: " << MockService::constructionCount << " (should be 1)\n";

        std::sort(latencies.begin(), latencies.end());
        std::cout << "    p99:         " << latencies[latencies.size() * 99 / 100] << " μs\n";
    }

    // --- Transient resolve ---
    {
        MockService::constructionCount = 0;
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < ITER; ++i) {
            auto svc = container.resolveTransient();
            (void)svc->value;
        }
        auto total = timer.elapsed();

        std::cout << "\n  Transient resolve:\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";
        std::cout << "    constructions: " << MockService::constructionCount << " (should be " << ITER << ")\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
