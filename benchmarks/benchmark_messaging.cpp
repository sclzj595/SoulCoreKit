// ============================================================================
// benchmark_messaging.cpp — MessageBus 基准 [v2.9.1]
// ============================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <atomic>
#include <memory>
#include "soul/event/inmemory_message_bus.h"

using namespace sc;

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== MessageBus Benchmark ===\n";

    const int ITER = 100000;

    // --- Subscribe ---
    {
        auto bus = std::make_shared<InMemoryMessageBus>();

        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < ITER; ++i) {
            auto sub = bus->subscribe(QString("ch.%1").arg(i % 100),
                [](const auto&) {});
        }
        auto total = timer.elapsed();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  Subscribe (100 channels):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";
    }

    // --- Publish (1 subscriber) ---
    {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> count{0};
        bus->subscribe("test", [&count](const auto&) { count++; });

        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            bus->publish("test", Message::create("test"));
            latencies.push_back(op.nsecsElapsed());
        }
        auto total = timer.elapsed();

        std::cout << "\n  Publish (1 subscriber):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";
        std::cout << "    delivered:   " << count.load() << "\n";

        std::sort(latencies.begin(), latencies.end());
        std::cout << "    p50:         " << latencies[latencies.size() / 2] << " ns\n";
        std::cout << "    p99:         " << latencies[latencies.size() * 99 / 100] << " ns\n";
    }

    // --- Publish (10 subscribers) ---
    {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> count{0};
        for (int s = 0; s < 10; ++s) {
            bus->subscribe("test", [&count](const auto&) { count++; });
        }

        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < ITER / 10; ++i) {
            bus->publish("test", Message::create("test"));
        }
        auto total = timer.elapsed();
        int iter10 = ITER / 10;

        std::cout << "\n  Publish (10 subscribers):\n";
        std::cout << "    iterations:  " << iter10 << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << iter10 * 1000.0 / total << " ops/sec\n";
        std::cout << "    delivered:   " << count.load() << " (should be " << iter10 * 10 << ")\n";
    }

    // --- Unsubscribe ---
    {
        auto bus = std::make_shared<InMemoryMessageBus>();
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < ITER; ++i) {
            auto sub = bus->subscribe("test", [](const auto&) {});
            bus->unsubscribe(sub);
        }
        auto total = timer.elapsed();

        std::cout << "\n  Subscribe + Unsubscribe:\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
    }

    // --- Message create ---
    {
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < ITER; ++i) {
            auto msg = Message::create("bench", QByteArray("payload"));
            (void)msg.id;
        }
        auto total = timer.elapsed();

        std::cout << "\n  Message::create():\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
