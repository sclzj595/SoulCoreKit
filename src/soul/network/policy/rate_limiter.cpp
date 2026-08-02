#include "soul/network/policy/rate_limiter.h"
#include <algorithm>
#include <cmath>

namespace sc {
namespace network {

RateLimiter::RateLimiter(Algorithm algorithm, double permitsPerSec, int burstSize)
    : m_algorithm(algorithm)
    , m_permitsPerSec(permitsPerSec > 0 ? permitsPerSec : 1.0)
    , m_burstSize(burstSize > 0 ? burstSize : static_cast<int>(permitsPerSec > 0 ? permitsPerSec : 1.0))
    , m_tokens(static_cast<double>(m_burstSize))
    , m_lastRefillTime(std::chrono::steady_clock::now()) {
    if (algorithm == Algorithm::SlidingWindow) {
        m_windowStartMs.store(0, std::memory_order_relaxed);
        m_windowCount.store(0, std::memory_order_relaxed);
    }
}

bool RateLimiter::tryAcquire() {
    return tryAcquire(1);
}

bool RateLimiter::tryAcquire(int permits) {
    if (permits <= 0) return true;

    switch (m_algorithm) {
    case Algorithm::TokenBucket:
        return tryAcquireTokenBucket(permits);
    case Algorithm::SlidingWindow:
        return tryAcquireSlidingWindow(permits);
    }
    return false;
}

// ============================================================================
// 令牌桶实现
// ============================================================================

void RateLimiter::refillTokens() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastRefillTime).count();

    if (elapsed <= 0) return;

    // 生成令牌: elapsed_ms / 1000.0 * permitsPerSec
    double newTokens = static_cast<double>(elapsed) / 1000.0 * m_permitsPerSec;
    m_tokens = std::min(m_tokens + newTokens, static_cast<double>(m_burstSize));
    m_lastRefillTime = now;
}

bool RateLimiter::tryAcquireTokenBucket(int permits) {
    std::lock_guard<std::mutex> lock(m_mutex);
    refillTokens();

    if (m_tokens >= static_cast<double>(permits)) {
        m_tokens -= static_cast<double>(permits);
        m_accepted.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    m_rejected.fetch_add(1, std::memory_order_relaxed);
    return false;
}

double RateLimiter::availablePermits() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_algorithm == Algorithm::SlidingWindow) {
        double permitsPerSec = m_permitsPerSec;
        int64_t maxPermits = static_cast<int64_t>(permitsPerSec);
        int64_t current = m_windowCount.load(std::memory_order_acquire);
        return static_cast<double>(std::max(int64_t(0), maxPermits - current));
    }
    refillTokens();
    return m_tokens;
}

void RateLimiter::setPermitsPerSec(double permitsPerSec) {
    if (permitsPerSec <= 0 || !std::isfinite(permitsPerSec)) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_permitsPerSec = permitsPerSec;
    m_tokens = std::min(m_tokens, static_cast<double>(m_burstSize));
}

void RateLimiter::setBurstSize(int burstSize) {
    if (burstSize <= 0) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_burstSize = burstSize;
    m_tokens = std::min(m_tokens, static_cast<double>(m_burstSize));
}

void RateLimiter::setWindowDurationMs(int windowMs) {
    if (windowMs <= 0) return;
    m_windowDurationMs.store(windowMs, std::memory_order_release);
}

int RateLimiter::windowDurationMs() const {
    return m_windowDurationMs.load(std::memory_order_acquire);
}

// ============================================================================
// 滑动窗口实现
// ============================================================================

bool RateLimiter::tryAcquireSlidingWindow(int permits) {
    double permitsPerSec;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        permitsPerSec = m_permitsPerSec;
    }
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    int64_t windowMs = m_windowDurationMs.load(std::memory_order_acquire);
    int64_t maxPermits = static_cast<int64_t>(permitsPerSec);

    int64_t expected = m_windowStartMs.load(std::memory_order_acquire);
    while (true) {
        if (nowMs - expected >= windowMs) {
            // 窗口过期,尝试重置
            if (m_windowStartMs.compare_exchange_weak(
                    expected, nowMs, std::memory_order_acq_rel, std::memory_order_acquire)) {
                // 重置窗口计数, 两步非原子但安全: 最坏情况是某个并发请求在重置前读到旧计数被误拒
                m_windowCount.store(0, std::memory_order_release);
                // CAS 成功: expected 仍为旧值,但 m_windowStartMs 已更新为 nowMs。
                // 直接进入计数逻辑(下一轮 expected 会被重新加载为 nowMs,无需浪费一次 CAS 重试)。
                // 注意: 此时 expected 未更新,但后续窗口过期检查会自然通过(nowMs - expected >= 0 < windowMs)。
            }
            // 无论 CAS 成功与否,都需要重新加载 expected 以反映最新窗口起始时间
            expected = m_windowStartMs.load(std::memory_order_acquire);
            continue;
        }

        // 窗口内,检查计数
        int64_t current = m_windowCount.load(std::memory_order_acquire);
        if (current + permits <= maxPermits) {
            if (m_windowCount.compare_exchange_weak(
                    current, current + permits, std::memory_order_acq_rel, std::memory_order_acquire)) {
                m_accepted.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
            // CAS 失败,重试
            continue;
        }

        m_rejected.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
}

} // namespace network
} // namespace sc