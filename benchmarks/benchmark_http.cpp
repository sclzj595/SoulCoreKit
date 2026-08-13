// ============================================================================
// benchmark_http.cpp — HTTP 路由匹配 + 请求处理基准 [v2.8.0]
// ============================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <functional>

// 模拟 HTTP 路由匹配
struct RouteKey {
    std::string method;
    std::string path;
    bool operator<(const RouteKey& o) const {
        if (method != o.method) return method < o.method;
        return path < o.path;
    }
};

using Handler = std::function<void()>;

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== HTTP Benchmark ===\n";

    const int ITER = 100000;

    // --- Route match (map lookup) ---
    {
        std::map<RouteKey, Handler> routes;
        routes[{"GET", "/api/health"}] = []{};
        routes[{"GET", "/api/users"}] = []{};
        routes[{"POST", "/api/users"}] = []{};
        routes[{"GET", "/api/users/42"}] = []{};
        routes[{"DELETE", "/api/users/42"}] = []{};
        routes[{"GET", "/api/ready"}] = []{};

        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            auto it = routes.find({"GET", "/api/users"});
            if (it != routes.end()) it->second();
            latencies.push_back(op.nsecsElapsed());
        }
        auto total = timer.elapsed();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  Route match (std::map, 6 routes):\n";
        std::cout << "    iterations:  " << ITER << "\n";
        std::cout << "    total:       " << total << " ms\n";
        std::cout << "    throughput:  " << ITER * 1000.0 / total << " ops/sec\n";
        std::cout << "    avg latency: " << total * 1e6 / ITER << " ns/op\n";

        std::sort(latencies.begin(), latencies.end());
        std::cout << "    p50:         " << latencies[latencies.size() / 2] << " ns\n";
        std::cout << "    p99:         " << latencies[latencies.size() * 99 / 100] << " ns\n";
    }

    // --- Path split + match (模拟 :id 路由) ---
    {
        QElapsedTimer timer;
        std::vector<int64_t> latencies;
        latencies.reserve(ITER);

        const QString path = "/api/users/42";

        timer.start();
        for (int i = 0; i < ITER; ++i) {
            QElapsedTimer op;
            op.start();
            auto parts = path.split('/');
            bool match = (parts.size() == 4 &&
                          parts[1] == "api" &&
                          parts[2] == "users" &&
                          !parts[3].isEmpty());
            (void)match;
            latencies.push_back(op.nsecsElapsed());
        }
        auto total = timer.elapsed();

        std::cout << "\n  Path split + match (4 segments):\n";
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
