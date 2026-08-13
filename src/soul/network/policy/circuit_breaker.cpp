#include "soul/network/policy/circuit_breaker.h"
#include "soul/logging/log_macros.h"

namespace sc {
namespace network {

CircuitBreaker::CircuitBreaker(std::string name, CircuitBreakerConfig config)
    : m_name(std::move(name))
    , m_config(config)
    , m_openedAt(std::chrono::steady_clock::time_point{}) {
}

CircuitBreaker& CircuitBreaker::setFailureThreshold(int threshold) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_config.failureThreshold = (threshold > 0) ? threshold : m_config.failureThreshold;
    return *this;
}

CircuitBreaker& CircuitBreaker::setResetTimeout(int ms) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // v3.0.0: 允许 0 值 (0 表示熔断后立即进入 Half-Open 试探)。
    // 原实现 (ms > 0) 会忽略 0, 导致测试 setResetTimeout(0) 无法立即半开。
    m_config.resetTimeoutMs = (ms >= 0) ? ms : m_config.resetTimeoutMs;
    return *this;
}

CircuitBreaker& CircuitBreaker::setHalfOpenMaxCalls(int maxCalls) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_config.halfOpenMaxCalls = (maxCalls > 0) ? maxCalls : m_config.halfOpenMaxCalls;
    return *this;
}

CircuitBreaker& CircuitBreaker::setWindowDuration(int ms) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_config.windowDurationMs = (ms > 0) ? ms : m_config.windowDurationMs;
    return *this;
}

CircuitBreaker& CircuitBreaker::setStateChangeCallback(StateChangeCallback cb) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_stateChangeCallback = std::move(cb);
    return *this;
}

CircuitBreakerState CircuitBreaker::state() const {
    return m_state.load(std::memory_order_acquire);
}

bool CircuitBreaker::allowRequest() {
    CircuitBreakerState current = m_state.load(std::memory_order_acquire);

    switch (current) {
    case CircuitBreakerState::Closed:
        return true;

    case CircuitBreakerState::Open: {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_openedAt).count();
        if (elapsed >= m_config.resetTimeoutMs) {
            // v3.0.0: 进入 HalfOpen 时重置成功试探计数 (m_halfOpenCalls 语义改为成功数)
            m_halfOpenCalls.store(0, std::memory_order_release);
            transitionTo(CircuitBreakerState::HalfOpen);
            return true;
        }
        return false;
    }

    case CircuitBreakerState::HalfOpen: {
        // 持锁防止与 onSuccess()/onFailure() 的状态转换竞态
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        // 重检状态: 持锁期间状态可能已被 onSuccess()/onFailure() 改变
        if (m_state.load(std::memory_order_acquire) != CircuitBreakerState::HalfOpen) {
            return (m_state.load(std::memory_order_acquire) == CircuitBreakerState::Closed);
        }
        // v3.0.0: m_halfOpenCalls 语义改为"已成功试探数"(见 onSuccess),
        // 此处 HalfOpen 状态直接放行 (成功/失败在 onSuccess/onFailure 中累计)。
        return true;
    }
    }

    return false;
}

void CircuitBreaker::onSuccess() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    CircuitBreakerState current = m_state.load(std::memory_order_acquire);
    if (current == CircuitBreakerState::HalfOpen) {
        // v3.0.0: 半开状态下的成功: 累计成功试探数。
        // 连续 halfOpenMaxCalls 次成功才恢复到 Closed (符合 Resilience4j 语义)。
        int successes = m_halfOpenCalls.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (successes >= m_config.halfOpenMaxCalls) {
            // 所有试探都成功了,恢复到 Closed
            m_failureTimestamps.clear();
            m_failureCount.store(0, std::memory_order_release);
            transitionTo(CircuitBreakerState::Closed);
        }
    } else if (current == CircuitBreakerState::Closed) {
        // 仅修剪过期失败记录,不清空窗口(防止间歇性成功导致熔断失效)
        pruneExpiredFailures();
    }
}

void CircuitBreaker::onFailure() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    CircuitBreakerState current = m_state.load(std::memory_order_acquire);

    // 记录失败时间戳到滑动窗口
    auto now = std::chrono::steady_clock::now();
    m_failureTimestamps.push_back(now);
    pruneExpiredFailures();

    switch (current) {
    case CircuitBreakerState::Closed: {
        // m_failureCount 已由 pruneExpiredFailures() 同步为 deque.size()
        int count = m_failureCount.load(std::memory_order_acquire);
        if (count >= m_config.failureThreshold) {
            transitionTo(CircuitBreakerState::Open);
        }
        break;
    }
    case CircuitBreakerState::HalfOpen:
        // 半开状态下失败: 立即回到 Open
        m_halfOpenCalls.store(0, std::memory_order_release);
        transitionTo(CircuitBreakerState::Open);
        break;
    case CircuitBreakerState::Open:
        // 已熔断,忽略
        break;
    }
}

void CircuitBreaker::reset() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_failureTimestamps.clear();
    m_failureCount.store(0, std::memory_order_release);
    m_halfOpenCalls.store(0, std::memory_order_release);
    transitionTo(CircuitBreakerState::Closed);
}

void CircuitBreaker::forceOpen() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    transitionTo(CircuitBreakerState::Open);
}

void CircuitBreaker::transitionTo(CircuitBreakerState newState) {
    CircuitBreakerState old = m_state.exchange(newState, std::memory_order_acq_rel);
    if (old == newState) return;

    if (newState == CircuitBreakerState::Open) {
        m_openedAt = std::chrono::steady_clock::now();
    }

    SC_LOGC_INFO(network, QString("CircuitBreaker '%1': %2 -> %3")
        .arg(QString::fromStdString(m_name))
        .arg(QString::fromUtf8(toString(old)))
        .arg(QString::fromUtf8(toString(newState))));

    if (m_stateChangeCallback) {
        m_stateChangeCallback(m_name, old, newState);
    }
}

void CircuitBreaker::pruneExpiredFailures() {
    auto now = std::chrono::steady_clock::now();
    // 移除窗口外的旧失败记录
    while (!m_failureTimestamps.empty()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_failureTimestamps.front()).count();
        if (elapsed >= m_config.windowDurationMs) {
            m_failureTimestamps.pop_front();
        } else {
            break;
        }
    }
    // 失败计数同步为窗口内实际记录数
    m_failureCount.store(static_cast<int>(m_failureTimestamps.size()), std::memory_order_release);
}

} // namespace network
} // namespace sc