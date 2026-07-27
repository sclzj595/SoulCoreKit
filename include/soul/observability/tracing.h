#ifndef SOUL_OBSERVABILITY_TRACING_H
#define SOUL_OBSERVABILITY_TRACING_H

#include <QString>
#include <QDateTime>
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sc {
namespace observability {

/**
 * @brief Span 状态
 */
enum class SpanStatus {
    Unset,      ///< 未设置
    Ok,         ///< 成功
    Error       ///< 错误
};

/**
 * @brief Span 类型
 */
enum class SpanKind {
    Internal,   ///< 内部操作（默认）
    Server,     ///< 服务端处理
    Client,     ///< 客户端调用
    Producer,   ///< 消息生产者
    Consumer    ///< 消息消费者
};

/**
 * @brief Span 事件（时间戳 + 名称 + 属性）
 */
struct SpanEvent {
    std::chrono::steady_clock::time_point timestamp;
    std::string name;
    std::map<std::string, std::string> attributes;
};

/**
 * @brief 追踪上下文（用于跨服务传播）
 *
 * 遵循 W3C Trace Context 规范：
 * - traceId: 32 字符十六进制（16 字节）
 * - spanId:  16 字符十六进制（8 字节）
 *
 * @par 使用示例
 * @code
 * // 服务端接收上下文
 * TraceContext ctx = TraceContext::fromHeader("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
 *
 * // 创建子 Span
 * auto span = Tracer::instance().startSpan("handle_request", ctx);
 * @endcode
 */
struct TraceContext {
    std::string traceId;    ///< 追踪 ID（整个链路共享）
    std::string spanId;     ///< 当前 Span ID
    std::string parentSpanId;  ///< 父 Span ID（根 Span 为空）

    /// @brief 生成新的 traceId（32 字符十六进制）
    [[nodiscard]] static std::string generateTraceId();

    /// @brief 生成新的 spanId（16 字符十六进制）
    [[nodiscard]] static std::string generateSpanId();

    /// @brief 序列化为 W3C traceparent 头格式
    /// @return 字符串如 "00-<traceId>-<spanId>-01"
    [[nodiscard]] std::string toTraceParent() const;

    /// @brief 从 W3C traceparent 头解析
    /// @return 解析成功返回 context，失败返回空 traceId
    [[nodiscard]] static TraceContext fromTraceParent(const std::string& header);

    /// @brief 是否为有效上下文
    [[nodiscard]] bool isValid() const noexcept {
        return !traceId.empty() && !spanId.empty();
    }
};

/**
 * @class Span
 * @brief 链路追踪跨度
 *
 * 表示一个独立的操作单元，记录：
 * - 操作名、开始/结束时间、持续时间
 * - 标签（key-value 属性）
 * - 事件（带时间戳的日志）
 * - 父子关系
 * - 状态（Ok/Error）
 *
 * @par 使用示例
 * @code
 * auto span = Tracer::instance().startSpan("database_query");
 * span->setAttribute("db.statement", "SELECT * FROM users");
 * span->setAttribute("db.duration_ms", 42);
 * span->addEvent("cache_miss");
 * // ... 执行查询 ...
 * span->setStatus(SpanStatus::Ok);
 * span->end();
 * @endcode
 *
 * @thread_safety Thread-Safe — 单个 Span 实例可被多线程访问
 */
class Span {
public:
    Span(std::string name, TraceContext context, SpanKind kind = SpanKind::Internal);

    ~Span();

    /// @brief 添加标签（字符串值）
    void setAttribute(const std::string& key, const std::string& value);

    /// @brief 添加标签（C 字符串字面量,避免被 bool 重载误匹配）
    void setAttribute(const std::string& key, const char* value);

    /// @brief 添加标签（数值值）
    void setAttribute(const std::string& key, double value);

    /// @brief 添加标签（布尔值）
    void setAttribute(const std::string& key, bool value);

    /// @brief 添加事件
    void addEvent(const std::string& name);

    /// @brief 添加事件（带属性）
    void addEvent(const std::string& name, const std::map<std::string, std::string>& attributes);

    /// @brief 设置状态
    void setStatus(SpanStatus status, const std::string& description = "");

    /// @brief 结束 Span（记录结束时间）
    void end();

    /// @brief Span 是否已结束
    [[nodiscard]] bool isEnded() const { return m_ended.load(std::memory_order_relaxed); }

    /// @brief 获取持续时间（毫秒）
    [[nodiscard]] std::chrono::milliseconds duration() const;

    // ===== 访问器 =====
    [[nodiscard]] const std::string& name() const { return m_name; }
    [[nodiscard]] const TraceContext& context() const { return m_context; }
    [[nodiscard]] SpanKind kind() const { return m_kind; }
    // TSan-safe: status/statusDescription/endTime are written under m_mutex by
    // setStatus()/end(); readers must also take the lock to establish happens-before.
    [[nodiscard]] SpanStatus status() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_status;
    }
    [[nodiscard]] std::string statusDescription() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_statusDescription;
    }
    [[nodiscard]] std::chrono::steady_clock::time_point startTime() const { return m_startTime; }
    [[nodiscard]] std::chrono::steady_clock::time_point endTime() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_endTime;
    }
    [[nodiscard]] std::map<std::string, std::string> stringAttributes() const;
    [[nodiscard]] std::map<std::string, double> numericAttributes() const;
    [[nodiscard]] std::vector<SpanEvent> events() const;

private:
    std::string                       m_name;
    TraceContext                      m_context;
    SpanKind                          m_kind;
    SpanStatus                        m_status = SpanStatus::Unset;
    std::string                       m_statusDescription;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_endTime;
    std::atomic<bool>                 m_ended{false};
    mutable std::mutex                m_mutex;
    std::map<std::string, std::string> m_stringAttributes;
    std::map<std::string, double>     m_numericAttributes;
    std::vector<SpanEvent>            m_events;
};

/**
 * @class SpanGuard
 * @brief RAII Span 管理（自动结束）
 *
 * 构造时创建 Span，析构时自动 end()。
 * 简化异常安全代码中的 Span 管理。
 *
 * @par 使用示例
 * @code
 * void handleRequest() {
 *     SpanGuard span(Tracer::instance().startSpan("handleRequest"));
 *     span->setAttribute("handler", "UserController");
 *     // ... 业务逻辑 ...
 *     // 析构时自动 end()
 * }
 * @endcode
 */
class SpanGuard {
public:
    explicit SpanGuard(std::shared_ptr<Span> span) : m_span(std::move(span)) {}
    ~SpanGuard() {
        if (m_span && !m_span->isEnded()) {
            m_span->end();
        }
    }

    SpanGuard(const SpanGuard&) = delete;
    SpanGuard& operator=(const SpanGuard&) = delete;
    SpanGuard(SpanGuard&&) = default;
    SpanGuard& operator=(SpanGuard&&) = default;

    Span* operator->() const { return m_span.get(); }
    Span& operator*() const { return *m_span; }
    [[nodiscard]] std::shared_ptr<Span> span() const { return m_span; }

private:
    std::shared_ptr<Span> m_span;
};

/**
 * @class Tracer
 * @brief 追踪器（单例）
 *
 * 全局追踪中心，负责创建 Span 和管理追踪上下文。
 *
 * @par 使用示例
 * @code
 * // 创建根 Span
 * auto rootSpan = Tracer::instance().startSpan("operation");
 *
 * // 创建子 Span（自动继承父上下文）
 * auto childSpan = Tracer::instance().startSpan("subtask", rootSpan.get());
 *
 * // 从远程上下文创建 Span
 * TraceContext remoteCtx = TraceContext::fromTraceParent(header);
 * auto span = Tracer::instance().startSpan("handle_remote", remoteCtx);
 * @endcode
 *
 * @thread_safety Thread-Safe
 */
class Tracer {
public:
    /// @brief 获取单例
    static Tracer& instance();

    /// @brief 创建根 Span（新 traceId）
    std::shared_ptr<Span> startSpan(const std::string& name,
                                     SpanKind kind = SpanKind::Internal);

    /// @brief 创建子 Span（继承父 Span 的 traceId）
    std::shared_ptr<Span> startSpan(const std::string& name,
                                     const Span& parent,
                                     SpanKind kind = SpanKind::Internal);

    /// @brief 从外部上下文创建 Span（用于跨服务追踪）
    std::shared_ptr<Span> startSpan(const std::string& name,
                                     const TraceContext& parentContext,
                                     SpanKind kind = SpanKind::Internal);

    /// @brief 获取所有已结束的 Span（用于导出）
    [[nodiscard]] std::vector<std::shared_ptr<Span>> endedSpans() const;

    /// @brief 清空所有 Span（仅用于测试）
    void clear();

    /// @brief 设置是否启用追踪（禁用时 startSpan 返回 nullptr）
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief 是否启用追踪
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /// @brief 设置 m_spans 容量上限（防止内存无限增长）
    /// @param maxSpans 最大保留 Span 数;0 表示无限制(不推荐,仅用于测试)
    /// @note 达到上限时,新 Span 入队会淘汰最旧的 Span(FIFO)
    void setMaxSpans(std::size_t maxSpans) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxSpans = maxSpans;
        evictIfNeededUnlocked();
    }

    /// @brief 获取当前容量上限
    [[nodiscard]] std::size_t maxSpans() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_maxSpans;
    }

private:
    Tracer() = default;
    Tracer(const Tracer&) = delete;
    Tracer& operator=(const Tracer&) = delete;

    /// @brief 淘汰最旧 Span 直到 m_spans.size() < m_maxSpans (调用者持锁)
    void evictIfNeededUnlocked() {
        if (m_maxSpans == 0) return;  // 0 表示无限制
        while (m_spans.size() >= m_maxSpans && !m_spans.empty()) {
            m_spans.erase(m_spans.begin());
        }
    }

    mutable std::mutex m_mutex;
    std::vector<std::shared_ptr<Span>> m_spans;
    std::atomic<bool> m_enabled{true};
    std::size_t m_maxSpans = kDefaultMaxSpans;  ///< 默认上限,防止内存泄漏

public:
    /// @brief 默认 Span 保留上限(可覆盖)
    static constexpr std::size_t kDefaultMaxSpans = 10000;
};

} // namespace observability
} // namespace sc

#endif // SOUL_OBSERVABILITY_TRACING_H
