#ifndef SOUL_NETWORK_POLICY_CIRCUIT_BREAKER_H
#define SOUL_NETWORK_POLICY_CIRCUIT_BREAKER_H

// ============================================================================
// circuit_breaker.h — 三态熔断器 [v1.9.3]
// ============================================================================
//
// 设计目标: 对标 Resilience4j CircuitBreaker,在网络调用失败率过高时
// 自动熔断,防止级联故障。下游服务恢复后自动半开试探。
//
// 三态机:
//   Closed → Open:     失败次数达到 failureThreshold 且时间窗口内
//   Open   → Half-Open: 等待 resetTimeoutMs 后自动切换
//   Half-Open → Closed: halfOpenMaxCalls 次试探全部成功
//   Half-Open → Open:   任意一次试探失败
//
// 配置:
//   - failureThreshold:  失败次数阈值(默认 5)
//   - resetTimeoutMs:    熔断恢复超时(默认 30000ms)
//   - halfOpenMaxCalls:  半开最大试探次数(默认 3)
//   - windowDurationMs:  滑动窗口大小(默认 60000ms)
//
// 用法:
//   CircuitBreaker cb("db-service");
//   cb.setFailureThreshold(3).setResetTimeout(15000);
//
//   auto result = cb.call([&]() {
//       return httpClient.send(request);
//   });
//   if (!result.isOk()) {
//       // 处理熔断或业务错误
//   }
//
//   // 集成到 HttpClient
//   httpClient.setCircuitBreaker(cb);
//
// @thread_safety Thread-Safe — 所有公开方法可并发调用

#include "soul/core/result.h"
#include "soul/core/error.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <string>

namespace sc {
namespace network {

// ============================================================================
// CircuitBreakerState — 熔断器状态
// ============================================================================
enum class CircuitBreakerState {
    Closed,    ///< 正常,请求通过
    Open,      ///< 熔断,拒绝请求
    HalfOpen   ///< 半开,试探性放行
};

inline const char* toString(CircuitBreakerState s) {
    switch (s) {
        case CircuitBreakerState::Closed:   return "CLOSED";
        case CircuitBreakerState::Open:     return "OPEN";
        case CircuitBreakerState::HalfOpen: return "HALF_OPEN";
    }
    return "UNKNOWN";
}

// ============================================================================
// CircuitBreakerConfig — 熔断器配置
// ============================================================================
struct CircuitBreakerConfig {
    int failureThreshold   = 5;       ///< 失败次数阈值
    int resetTimeoutMs     = 30000;   ///< 熔断恢复超时(ms)
    int halfOpenMaxCalls   = 3;       ///< 半开最大试探次数
    int windowDurationMs   = 60000;   ///< 滑动窗口大小(ms),超过此窗口的失败记录不计入阈值
};

// ============================================================================
// CircuitBreaker — 熔断器
// ============================================================================
class CircuitBreaker {
public:
    using StateChangeCallback = std::function<void(const std::string& name,
                                                    CircuitBreakerState from,
                                                    CircuitBreakerState to)>;

    explicit CircuitBreaker(std::string name,
                            CircuitBreakerConfig config = {});
    ~CircuitBreaker() = default;

    CircuitBreaker(const CircuitBreaker&) = delete;
    CircuitBreaker& operator=(const CircuitBreaker&) = delete;

    // ============================================================
    // 配置(链式调用)
    // ============================================================
    CircuitBreaker& setFailureThreshold(int threshold);
    CircuitBreaker& setResetTimeout(int ms);
    CircuitBreaker& setHalfOpenMaxCalls(int maxCalls);
    CircuitBreaker& setWindowDuration(int ms);
    CircuitBreaker& setStateChangeCallback(StateChangeCallback cb);

    // ============================================================
    // 核心方法
    // ============================================================

    /// @brief 包装调用,自动处理熔断逻辑
    /// @tparam F 可调用对象,返回 Result<T>
    /// @return 调用结果。熔断时返回 Error(ErrorCode::ResourceExhausted, ...)
    template<typename F>
    auto call(F&& func) -> decltype(func()) {
        using ResultType = decltype(func());

        // 1. 检查是否允许调用
        if (!allowRequest()) {
            return ResultType::err(Error(
                ErrorCode::ResourceExhausted,
                QString("CircuitBreaker '%1' is OPEN").arg(QString::fromStdString(m_name))
            ));
        }

        // 2. 执行调用
        auto result = func();

        // 3. 记录结果(成功/失败)
        if (result.isOk()) {
            onSuccess();
        } else {
            onFailure();
        }

        return result;
    }

    /// @brief 手动标记成功(用于回调式异步调用)
    void onSuccess();

    /// @brief 手动标记失败(用于回调式异步调用)
    void onFailure();

    /// @brief 检查是否允许请求通过
    bool allowRequest();

    // ============================================================
    // 状态查询
    // ============================================================
    CircuitBreakerState state() const;
    const std::string& name() const { return m_name; }
    CircuitBreakerConfig config() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_config;
    }

    /// @brief 重置到 Closed 状态(用于测试/手动恢复)
    void reset();

    /// @brief 强制切换到 Open 状态(用于手动熔断)
    void forceOpen();

private:
    /// @brief 状态转换
    void transitionTo(CircuitBreakerState newState);

    /// @brief 清理过期失败记录
    void pruneExpiredFailures();

    std::string              m_name;
    CircuitBreakerConfig     m_config;
    mutable std::recursive_mutex       m_mutex;
    std::atomic<CircuitBreakerState> m_state{CircuitBreakerState::Closed};
    std::atomic<int>         m_failureCount{0};
    std::atomic<int>         m_halfOpenCalls{0};
    std::deque<std::chrono::steady_clock::time_point> m_failureTimestamps;  ///< 滑动窗口内失败时间戳
    std::chrono::steady_clock::time_point m_openedAt;
    StateChangeCallback      m_stateChangeCallback;
};

} // namespace network
} // namespace sc

#endif // SOUL_NETWORK_POLICY_CIRCUIT_BREAKER_H