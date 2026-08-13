// ============================================================================
// benchmark_configuration.cpp — Config 基准 [v2.9.0]
// ============================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <QHash>
#include <QVariant>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== Configuration Benchmark ===\n";

    const int ITER = 100000;

    // --- QHash read (模拟 Snapshot getString) ---
    {
        QHash<QString, QVariant> config;
        config["server.host"] = "0.0.0.0";
        config["server.port"] = 8080;
        config["database.host"] = "localhost";
        config["database.port"] = 3306;
        config["cache.enabled"] = true;
        config["timeout"] = 30.0;

        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            auto it = config.find("server.port");
            if (it != config.end()) {
                (void)it->toInt();
            }
            latencies.push_back(op.nsecsElapsed());
        }
        auto total = timer.elapsed();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  QHash read (6 keys):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";

        std::sort(latencies.begin(), latencies.end());
        std::cout << "    p50:         " << latencies[latencies.size() / 2] << " ns\n";
        std::cout << "    p99:         " << latencies[latencies.size() * 99 / 100] << " ns\n";
    }

    // --- QHash merge (模拟 Snapshot merge) ---
    {
        QHash<QString, QVariant> base;
        for (int i = 0; i < 20; ++i) {
            base[QString("key.%1").arg(i)] = i;
        }

        QHash<QString, QVariant> overlay;
        for (int i = 10; i < 15; ++i) {
            overlay[QString("key.%1").arg(i)] = i * 100;
        }

        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QHash<QString, QVariant> merged = base;
            for (auto it = overlay.begin(); it != overlay.end(); ++it) {
                merged.insert(it.key(), it.value());
            }
        }
        auto total = timer.elapsed();

        std::cout << "\n  QHash merge (20 base + 5 overlay):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";
    }

    // --- ConfigSnapshot construct (模拟 Snapshot 构造) ---
    {
        QHash<QString, QVariant> values;
        for (int i = 0; i < 50; ++i) {
            values[QString("config.key.%1").arg(i)] = i;
        }

        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < ITER / 10; ++i) {
            QHash<QString, QVariant> copy = values;  // 模拟 Snapshot 拷贝
            (void)copy.size();
        }
        auto total = timer.elapsed();

        int iter10 = ITER / 10;
        std::cout << "\n  ConfigSnapshot construct (50 keys):\n";
        std::cout << "    iterations:  " << iter10 << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << iter10 * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / iter10 << " ns/op\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
