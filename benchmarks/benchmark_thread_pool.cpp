#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <atomic>
#include <vector>
#include "soul/async/thread_pool.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "=== ThreadPool Benchmark ===" << std::endl;

    QElapsedTimer timer;
    const int iterations = 100000;
    std::atomic<int> counter(0);

    timer.start();
    for (int i = 0; i < iterations; ++i) {
        sc::ThreadPool::instance().start([&counter]() {
            ++counter;
        });
    }
    sc::ThreadPool::instance().waitForDone();
    std::cout << "1. Submit " << iterations << " tasks: " << timer.elapsed() << " ms" << std::endl;

    const int batchSize = 1000;
    const int batches = 1000;
    counter = 0;

    timer.start();
    for (int b = 0; b < batches; ++b) {
        for (int i = 0; i < batchSize; ++i) {
            sc::ThreadPool::instance().start([&counter]() {
                ++counter;
            });
        }
        sc::ThreadPool::instance().waitForDone();
    }
    std::cout << "2. Submit " << batches << " batches of " << batchSize << ": " << timer.elapsed() << " ms" << std::endl;

    std::vector<int> results(iterations);
    counter = 0;

    timer.start();
    for (int i = 0; i < iterations; ++i) {
        sc::ThreadPool::instance().start([&results, i]() {
            results[i] = i * 2;
        });
    }
    sc::ThreadPool::instance().waitForDone();
    std::cout << "3. Submit " << iterations << " tasks with data processing: " << timer.elapsed() << " ms" << std::endl;

    int maxThreads[] = {1, 2, 4, 8, 16};
    for (int mt : maxThreads) {
        sc::ThreadPool::instance().setMaxThreadCount(mt);
        counter = 0;
        timer.start();
        for (int i = 0; i < iterations; ++i) {
            sc::ThreadPool::instance().start([&counter]() {
                ++counter;
            });
        }
        sc::ThreadPool::instance().waitForDone();
        std::cout << "4. " << mt << " threads, " << iterations << " tasks: " << timer.elapsed() << " ms" << std::endl;
    }

    std::cout << std::endl;
    return 0;
}