#ifndef SOUL_CORE_BENCHMARK_H
#define SOUL_CORE_BENCHMARK_H

// ============================================================================
// benchmark.h — 轻量级性能基准测试框架 [v1.9.2 新增]
// ============================================================================
//
// 设计目标: 提供零依赖的轻量级性能基准测试,无需引入 Google Benchmark。
// 支持函数级计时、统计汇总、多轮迭代。
//
// 设计原则:
//   - 零依赖: 仅使用 C++ 标准库
//   - 宏驱动: 类似 Google Benchmark 的声明式 API
//   - 自动统计: 自动计算平均值、最小值、最大值、中位数
//
// 用法:
//   SC_BENCHMARK(MyBenchmark) {
//       SC_BENCH_START;
//       // ... code to benchmark ...
//       SC_BENCH_STOP;
//   }
//
//   SC_BENCHMARK_ITER(MyBenchmark, 1000) {
//       // ... code to benchmark (runs 1000 iterations) ...
//   }

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

namespace sc {
namespace benchmark {

// ============================================================================
// BenchmarkResult — 基准测试结果
// ============================================================================
struct BenchmarkResult {
    std::string name;
    std::size_t iterations = 0;
    double totalTimeUs = 0.0;       ///< 总耗时(微秒)
    double avgTimeUs = 0.0;         ///< 平均耗时(微秒)
    double minTimeUs = 0.0;         ///< 最小耗时(微秒)
    double maxTimeUs = 0.0;         ///< 最大耗时(微秒)
    double medianTimeUs = 0.0;      ///< 中位数耗时(微秒)
    double stdDevUs = 0.0;          ///< 标准差(微秒)
    double throughputPerSec = 0.0;  ///< 每秒吞吐量

    void print() const {
        std::cout << "=== " << name << " ===" << std::endl;
        std::cout << "  Iterations:    " << iterations << std::endl;
        std::cout << "  Total:         " << totalTimeUs << " us" << std::endl;
        std::cout << "  Average:       " << avgTimeUs << " us" << std::endl;
        std::cout << "  Min:           " << minTimeUs << " us" << std::endl;
        std::cout << "  Max:           " << maxTimeUs << " us" << std::endl;
        std::cout << "  Median:        " << medianTimeUs << " us" << std::endl;
        std::cout << "  StdDev:        " << stdDevUs << " us" << std::endl;
        std::cout << "  Throughput:    " << throughputPerSec << " ops/sec" << std::endl;
        std::cout << std::endl;
    }
};

// ============================================================================
// BenchmarkRunner — 基准测试运行器
// ============================================================================
class BenchmarkRunner {
public:
    static BenchmarkRunner& instance() {
        static BenchmarkRunner inst;
        return inst;
    }

    /// @brief 注册基准测试
    void registerBenchmark(const std::string& name, std::function<void()> fn) {
        m_benchmarks.push_back({name, std::move(fn)});
    }

    /// @brief 运行所有注册的基准测试
    void runAll() {
        std::cout << "=== Running " << m_benchmarks.size() << " benchmarks ===" << std::endl;
        for (auto& [name, fn] : m_benchmarks) {
            fn();
        }
        std::cout << "=== All benchmarks completed ===" << std::endl;
    }

    /// @brief 运行指定名称的基准测试
    void run(const std::string& name) {
        for (auto& [n, fn] : m_benchmarks) {
            if (n == name) {
                fn();
                return;
            }
        }
    }

    /// @brief 记录结果
    void addResult(const BenchmarkResult& result) {
        m_results.push_back(result);
        result.print();
    }

    const std::vector<BenchmarkResult>& results() const { return m_results; }

private:
    BenchmarkRunner() = default;
    std::vector<std::pair<std::string, std::function<void()>>> m_benchmarks;
    std::vector<BenchmarkResult> m_results;
};

// ============================================================================
// 计时辅助函数
// ============================================================================

/// @brief 对函数进行指定次数的迭代计时
/// @param fn      待测试函数
/// @param iterations 迭代次数
/// @param name    测试名称
/// @return BenchmarkResult
inline BenchmarkResult measure(const std::string& name,
                                std::function<void()> fn,
                                std::size_t iterations = 1000) {
    std::vector<double> times;
    times.reserve(iterations);

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        auto iterStart = std::chrono::high_resolution_clock::now();
        fn();
        auto iterEnd = std::chrono::high_resolution_clock::now();
        times.push_back(
            std::chrono::duration<double, std::micro>(iterEnd - iterStart).count());
    }
    auto end = std::chrono::high_resolution_clock::now();

    BenchmarkResult result;
    result.name = name;
    result.iterations = iterations;
    result.totalTimeUs = std::chrono::duration<double, std::micro>(end - start).count();

    // 统计
    std::sort(times.begin(), times.end());
    result.minTimeUs = times.front();
    result.maxTimeUs = times.back();
    result.medianTimeUs = times[times.size() / 2];

    double sum = 0.0;
    for (double t : times) sum += t;
    result.avgTimeUs = sum / iterations;

    // 标准差
    double variance = 0.0;
    for (double t : times) {
        double diff = t - result.avgTimeUs;
        variance += diff * diff;
    }
    result.stdDevUs = std::sqrt(variance / iterations);

    result.throughputPerSec = (iterations / result.totalTimeUs) * 1'000'000.0;

    return result;
}

/// @brief 单次计时(返回微秒)
inline double timeOnce(std::function<void()> fn) {
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

} // namespace benchmark
} // namespace sc

// ============================================================================
// 基准测试宏
// ============================================================================

/// @brief 声明一个基准测试
#define SC_BENCHMARK(name) \
    static void _sc_bench_##name(); \
    namespace { \
        struct _sc_bench_reg_##name { \
            _sc_bench_reg_##name() { \
                sc::benchmark::BenchmarkRunner::instance().registerBenchmark( \
                    #name, _sc_bench_##name); \
            } \
        } _sc_bench_reg_inst_##name; \
    } \
    static void _sc_bench_##name()

/// @brief 带迭代次数的基准测试
#define SC_BENCHMARK_ITER(name, iters) \
    SC_BENCHMARK(name) { \
        sc::benchmark::BenchmarkRunner::instance().addResult( \
            sc::benchmark::measure(#name, []() {

/// @brief 结束迭代基准测试
#define SC_BENCHMARK_END }, iters)); \
    }

/// @brief 开始计时
#define SC_BENCH_START \
    auto _sc_bench_start = std::chrono::high_resolution_clock::now()

/// @brief 停止计时并输出
#define SC_BENCH_STOP \
    auto _sc_bench_end = std::chrono::high_resolution_clock::now(); \
    auto _sc_bench_us = std::chrono::duration<double, std::micro>( \
        _sc_bench_end - _sc_bench_start).count(); \
    std::cout << "[" << __func__ << "] " << _sc_bench_us << " us" << std::endl

#endif // SOUL_CORE_BENCHMARK_H