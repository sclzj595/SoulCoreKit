#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <atomic>
#include "soul/network/pool/connection_pool.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "=== ConnectionPool Benchmark ===" << std::endl;

    sc::network::ConnectionPool::Config config;
    config.maxConnections = 20;
    config.minConnections = 5;
    config.idleTimeoutMs = 30000;

    sc::network::ConnectionPool pool(config);
    QElapsedTimer timer;

    const int iterations = 1000;

    timer.start();
    for (int i = 0; i < iterations; ++i) {
        auto conn = pool.acquire(QUrl("http://localhost:8080"));
        pool.release(conn);
    }
    std::cout << "1. Acquire/Release " << iterations << " connections: " << timer.elapsed() << " ms" << std::endl;

    std::atomic<int> counter(0);
    const int parallelOps = 100;

    timer.start();
    for (int i = 0; i < parallelOps; ++i) {
        auto conn = pool.acquire(QUrl("http://localhost:8080"));
        ++counter;
        pool.release(conn);
    }
    std::cout << "2. Parallel acquire/release " << parallelOps << ": " << timer.elapsed() << " ms" << std::endl;

    std::cout << std::endl;
    return 0;
}