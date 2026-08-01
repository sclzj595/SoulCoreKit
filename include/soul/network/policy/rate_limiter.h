#ifndef SOUL_NETWORK_POLICY_RATE_LIMITER_H
#define SOUL_NETWORK_POLICY_RATE_LIMITER_H

// ============================================================================
// rate_limiter.h — 令牌桶限流器 [v1.9.3]
// ============================================================================
//
// 设计目标: 对标 Guava RateLimiter,提供令牌桶 + 滑动窗口两种限流算法,
// 保护服务不被突发流量打爆。
//
// 算法:
//   - TokenBucket: 以恒定速率生成令牌,请求消耗令牌。支持突发(burst)。
//   - SlidingWindow: 滑动时间窗口内计数,超过阈值拒绝。
//
// 用法:
//   // 令牌桶: 100 QPS, 允许突发 50
//   RateLimiter limiter(RateLimiter::Algorithm::TokenBucket, 100, 50);
//
//   if (limiter.tryAcquire()) {
//       // 处理请求
//   } else {
//       // 返回 429 Too Many Requests
//   }
//
//   // 作为 HTTP 中间件
//   server.use(std::make_shared<RateLimitMiddleware>(limiter));
//
// @thread_safety Thread-Safe — 所有公开方法可并发调用

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>

namespace sc {
namespace network {

// ============================================================================
// RateLimiter — 限流器
// ============================================================================
class RateLimiter {
public:
    enum class Algorithm {
        TokenBucket,    ///< 令牌桶(平滑突发)
        SlidingWindow   ///< 滑动窗口(精确计数)
    };

    /// @param algorithm     限流算法
    /// @param permitsPerSec 每秒允许的请求数(QPS)
    /// @param burstSize     突发容量(令牌桶的最大令牌数,默认 = permitsPerSec)
    explicit RateLimiter(Algorithm algorithm = Algorithm::TokenBucket,
                         double permitsPerSec = 100.0,
                         int burstSize = 0);

    /// @brief 尝试获取 1 个令牌/许可。非阻塞。
    /// @return true 允许通过, false 被限流
    bool tryAcquire();

    /// @brief 尝试获取指定数量的令牌
    /// @param permits 需要的令牌数
    bool tryAcquire(int permits);

    /// @brief 获取当前可用令牌数(仅 TokenBucket)
    double availablePermits() const;

    /// @brief 获取当前 QPS 设置
    double permitsPerSec() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_permitsPerSec;
    }

    /// @brief 动态调整 QPS(线程安全)
    void setPermitsPerSec(double permitsPerSec);

    /// @brief 获取被拒绝的请求总数
    uint64_t rejectedCount() const { return m_rejected.load(std::memory_order_relaxed); }

    /// @brief 获取通过的请求总数
    uint64_t acceptedCount() const { return m_accepted.load(std::memory_order_relaxed); }

private:
    // 令牌桶
    void refillTokens() const;
    bool tryAcquireTokenBucket(int permits);

    // 滑动窗口
    bool tryAcquireSlidingWindow(int permits);

    Algorithm           m_algorithm;
    double              m_permitsPerSec;
    int                 m_burstSize;
    mutable std::mutex  m_mutex;
    mutable double      m_tokens;           ///< 当前令牌数
    mutable std::chrono::steady_clock::time_point m_lastRefillTime;

    // 滑动窗口
    std::atomic<int64_t> m_windowStartMs{0};
    std::atomic<int64_t> m_windowCount{0};

    // 统计
    std::atomic<uint64_t> m_rejected{0};
    std::atomic<uint64_t> m_accepted{0};
};

} // namespace network
} // namespace sc

#endif // SOUL_NETWORK_POLICY_RATE_LIMITER_H