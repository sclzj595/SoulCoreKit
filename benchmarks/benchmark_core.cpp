// ============================================================================
// benchmark_core.cpp — Core 层基准 [v2.8.0]
// ============================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <numeric>
#include "soul/core/result.h"
#include "soul/core/error.h"

using namespace sc;

struct BenchResult {
    std::string name;
    int64_t totalMs;
    int64_t iterations;
    double opsPerSec;
    double nsPerOp;
    std::vector<int64_t> latencies;  // 每操作耗时 (μs)
};

void printResult(const BenchResult& r) {
    std::vector<int64_t> sorted = r.latencies;
    std::sort(sorted.begin(), sorted.end());

    auto p = [&sorted](double pct) -> double {
        return sorted[static_cast<size_t>(sorted.size() * pct)];
    };

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n  " << r.name << ":\n";
    std::cout << "    iterations:  " << r.iterations << "\n";
    std::cout << "    total:       " << r.totalMs << " ms\n";
    std::cout << "    throughput:  " << r.opsPerSec << " ops/sec\n";
    std::cout << "    avg latency: " << r.nsPerOp << " ns/op\n";
    std::cout << "    p50:         " << p(0.50) << " μs\n";
    std::cout << "    p95:         " << p(0.95) << " μs\n";
    std::cout << "    p99:         " << p(0.99) << " μs\n";
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== SoulCoreKit Core Benchmark ===\n";

    const int ITER = 100000;

    // --- Result<T>::ok() ---
    {
        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            auto r = Result<int>::ok(i);
            (void)r.unwrap();
            latencies.push_back(op.nsecsElapsed() / 1000);  // μs
        }
        auto total = timer.elapsed();

        BenchResult result;
        result.name = "Result<int>::ok()";
        result.totalMs = total;
        result.iterations = ITER;
        result.opsPerSec = ITER * 1000.0 / total;
        result.nsPerOp = total * 1e6 / ITER;
        result.latencies = latencies;
        printResult(result);
    }

    // --- Result<T>::map() ---
    {
        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            auto r = Result<int>::ok(i).map([](int v) { return v * 2; });
            (void)r.unwrap();
            latencies.push_back(op.nsecsElapsed() / 1000);
        }
        auto total = timer.elapsed();

        BenchResult result;
        result.name = "Result<int>::map(x2)";
        result.totalMs = total;
        result.iterations = ITER;
        result.opsPerSec = ITER * 1000.0 / total;
        result.nsPerOp = total * 1e6 / ITER;
        result.latencies = latencies;
        printResult(result);
    }

    // --- Error construction ---
    {
        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            Error err(ErrorCode::Timeout, "Connection timeout");
            (void)err.message();
            (void)err.code();
            latencies.push_back(op.nsecsElapsed() / 1000);
        }
        auto total = timer.elapsed();

        BenchResult result;
        result.name = "Error construction + access";
        result.totalMs = total;
        result.iterations = ITER;
        result.opsPerSec = ITER * 1000.0 / total;
        result.nsPerOp = total * 1e6 / ITER;
        result.latencies = latencies;
        printResult(result);
    }

    // --- Error withContext() ---
    {
        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            auto enriched = Error(ErrorCode::NetworkError, "test")
                .withContext("request_id", QString::number(i));
            (void)enriched.contextValue("request_id");
            latencies.push_back(op.nsecsElapsed() / 1000);
        }
        auto total = timer.elapsed();

        BenchResult result;
        result.name = "Error::withContext()";
        result.totalMs = total;
        result.iterations = ITER;
        result.opsPerSec = ITER * 1000.0 / total;
        result.nsPerOp = total * 1e6 / ITER;
        result.latencies = latencies;
        printResult(result);
    }

    std::cout << "\nDone.\n";
    return 0;
}
