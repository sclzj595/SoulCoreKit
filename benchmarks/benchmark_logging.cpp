// ============================================================================
// benchmark_logging.cpp — Logger 基准 [v2.8.0]
// ============================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== Logger Benchmark ===\n";

    const int ITER = 100000;

    // --- String formatting only (no logger) ---
    {
        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            std::string msg = "Benchmark message " + std::to_string(i);
            (void)msg;
            latencies.push_back(op.nsecsElapsed() / 1000);
        }
        auto total = timer.elapsed();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  String format (baseline):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";

        std::sort(latencies.begin(), latencies.end());
        std::cout << "    p99:         " << latencies[latencies.size() * 99 / 100] << " μs\n";
    }

    // --- Memory allocation benchmark ---
    {
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < ITER; ++i) {
            auto ptr = std::make_unique<int>(i);
            (void)*ptr;
        }
        auto total = timer.elapsed();

        std::cout << "\n  make_unique<int> (allocation):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
