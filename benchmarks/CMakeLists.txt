# ============================================================================
# SoulCoreKit Benchmarks — v2.8.0
# ============================================================================
# 用法:
#   cmake -DSOULCOREKIT_BUILD_BENCHMARKS=ON ..
#   cmake --build . --target all_benchmarks
#   ./bench_core
#
# 基准输出:
#   - Throughput (ops/sec)
#   - Latency (ns/op)
#   - p50 / p95 / p99
# ============================================================================

find_package(Qt6 REQUIRED COMPONENTS Core)

# 聚合目标
add_custom_target(all_benchmarks)

function(soul_benchmark BENCH_NAME)
    add_executable(${BENCH_NAME} ${BENCH_NAME}.cpp)
    target_link_libraries(${BENCH_NAME} PRIVATE
        Qt6::Core
        ${ARGN}
    )
    add_dependencies(all_benchmarks ${BENCH_NAME})
endfunction()

# --- Core 层基准 ---
soul_benchmark(benchmark_core
    soul_core
)

# --- DI 基准 ---
soul_benchmark(benchmark_di
    soul_di soul_core
)

# --- Event 基准 ---
soul_benchmark(benchmark_event
    soul_event soul_core
)

# --- Async 基准 ---
soul_benchmark(benchmark_thread_pool
    soul_async soul_core
)

# --- Logging 基准 ---
soul_benchmark(benchmark_logging
    soul_logging soul_core
)

# --- Metrics 基准 ---
soul_benchmark(benchmark_metrics
    soul_observability soul_core
)

# --- HTTP 基准 ---
soul_benchmark(benchmark_http
    soul_network soul_core
    Qt6::Network
)

# --- Cache 基准 [v3.0.0: 修复依赖声明] ---
soul_benchmark(benchmark_cache
    soul_cache soul_core
)

# --- ConnectionPool 基准 ---
soul_benchmark(benchmark_connection_pool
    soul_network soul_event soul_core soul_logging
    Qt6::Network
)

# --- Configuration 基准 [v2.9.0] ---
soul_benchmark(benchmark_configuration
    soul_configuration soul_core
)

# --- Messaging 基准 [v2.9.1] ---
soul_benchmark(benchmark_messaging
    soul_event soul_core
)
