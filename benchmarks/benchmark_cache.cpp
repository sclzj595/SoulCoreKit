#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <string>
#include <random>
// v3.0.0: migrated to canonical cache
#include "soul/cache/memory_cache.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "=== MemoryCache Benchmark ===" << std::endl;

    QElapsedTimer timer;
    const int iterations = 100000;
    const int maxSize = 10000;

    sc::cache::MemoryCache<std::string, std::string>::Config cfg;
    cfg.maxEntries = static_cast<std::size_t>(maxSize);
    sc::cache::MemoryCache<std::string, std::string> cache(cfg);

    timer.start();
    for (int i = 0; i < iterations; ++i) {
        cache.put("key_" + std::to_string(i), "value_" + std::to_string(i));
    }
    std::cout << "1. Put " << iterations << " entries: " << timer.elapsed() << " ms" << std::endl;
    std::cout << "   Cache size: " << cache.size() << std::endl;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, iterations - 1);

    timer.start();
    for (int i = 0; i < iterations; ++i) {
        int idx = dis(gen);
        auto result = cache.get("key_" + std::to_string(idx));
    }
    std::cout << "2. Get " << iterations << " random entries: " << timer.elapsed() << " ms" << std::endl;

    timer.start();
    for (int i = 0; i < iterations; ++i) {
        (void)cache.contains("key_" + std::to_string(i));
    }
    std::cout << "3. Contains " << iterations << " entries: " << timer.elapsed() << " ms" << std::endl;

    timer.start();
    for (int i = 0; i < iterations; ++i) {
        cache.remove("key_" + std::to_string(i));
    }
    std::cout << "4. Remove " << iterations << " entries: " << timer.elapsed() << " ms" << std::endl;

    sc::cache::MemoryCache<int, int>::Config intCfg;
    intCfg.maxEntries = 1000;
    sc::cache::MemoryCache<int, int> intCache(intCfg);
    timer.start();
    for (int i = 0; i < iterations; ++i) {
        intCache.put(i, i * 2);
    }
    std::cout << "5. Put " << iterations << " int entries: " << timer.elapsed() << " ms" << std::endl;

    timer.start();
    for (int i = 0; i < iterations; ++i) {
        auto result = intCache.get(i % 1000);
    }
    std::cout << "6. Get " << iterations << " int entries (with eviction): " << timer.elapsed() << " ms" << std::endl;

    std::cout << std::endl;
    return 0;
}