// ============================================================================
// benchmark_metrics.cpp — Metrics 基准 [v2.8.0]
// ============================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <vector>
#include <algorithm>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== Metrics Benchmark ===\n";

    const int ITER = 100000;

    // --- Atomic counter increment ---
    {
        std::atomic<int64_t> counter{0};

        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            counter.fetch_add(1, std::memory_order_relaxed);
            latencies.push_back(op.nsecsElapsed());
        }
        auto total = timer.elapsed();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  Atomic increment (Counter):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";
        std::cout << "    final value: " << counter.load() << "\n";

        std::sort(latencies.begin(), latencies.end());
        std::cout << "    p50:         " << latencies[latencies.size() / 2] << " ns\n";
        std::cout << "    p99:         " << latencies[latencies.size() * 99 / 100] << " ns\n";
    }

    // --- Mutex-protected histogram observe ---
    {
        // 模拟 Histogram::observe() 的最小开销
        std::mutex mtx;
        double sum = 0;
        int64_t count = 0;

        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            {
                std::lock_guard lock(mtx);
                sum += static_cast<double>(i % 100);
                count++;
            }
            latencies.push_back(op.nsecsElapsed());
        }
        auto total = timer.elapsed();

        std::cout << "\n  Mutex-protected observe (Histogram):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";

        std::sort(latencies.begin(), latencies.end());
        std::cout << "    p50:         " << latencies[latencies.size() / 2] << " ns\n";
        std::cout << "    p99:         " << latencies[latencies.size() * 99 / 100] << " ns\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
