#ifndef SOUL_CORE_REQUEST_CONTEXT_H
#define SOUL_CORE_REQUEST_CONTEXT_H

// ============================================================================
// request_context.h — 统一请求上下文 [v2.8.0 新增]
// ============================================================================
//
// 设计目标: 让 requestId / traceId / correlationId 能够贯穿一次请求的
// 完整生命周期，不依赖 HTTP 或任何特定协议。
//
// 使用模式:
//   1. HTTP Server: 在请求进入时创建 Context，请求结束时销毁
//   2. RPC Client:  在发起调用时创建 Context
//   3. CS Client:   在发起网络请求时创建 Context
//   4. CLI:         可选，用于日志关联
//
// 贯穿路径:
//   HTTP Request
//       ↓
//   RequestContext::create()        ← TraceMiddleware / LoggingMiddleware
//       ↓
//   RequestContext::current()       ← Controller / Service / Repository
//       ↓
//   SC_INFO_CTX(...)                ← Logger 自动关联 requestId/traceId
//       ↓
//   RequestContext::destroy()       ← 请求结束 (RAII ScopeGuard)
//
// 线程安全:
//   - 使用 thread_local 存储当前 Context (不跨线程共享)
//   - Context 对象本身是值语义 (copy 安全)
//   - 线程池复用场景: ScopeGuard 析构自动清理 thread_local
//
// 禁止:
//   - 跨线程传递 Context (使用 traceId 字符串传递)
//   - 在 thread_local 中泄漏 Context (ScopeGuard 保证清理)
//   - 把 userId/requestId 作为无限增长的 metric label
// ============================================================================

#include <QString>
#include <QDateTime>
#include <QHash>
#include <QVariant>
#include <chrono>
#include <memory>
#include <string>

namespace sc {

// ============================================================================
// RequestContext — 请求上下文 (值语义)
// ============================================================================

struct RequestContext {
    // --- 核心标识 ---
    QString requestId;       // 请求唯一 ID (UUID)
    QString traceId;         // 分布式追踪 ID
    QString spanId;          // 当前 Span ID
    QString correlationId;   // 关联 ID (跨服务传递)

    // --- 时间 ---
    std::chrono::steady_clock::time_point startTime;

    // --- 来源信息 ---
    QString sourceComponent;  // "http-server", "rpc-client", "cli"
    QString method;           // "GET", "POST", "RPC:GetUser"
    QString path;             // "/api/users", "GetUser"

    // --- 用户信息 (可选) ---
    QString userId;
    QString sessionId;

    // --- 扩展元数据 ---
    QHash<QString, QString> metadata;

    // ========================================================================
    // 工厂方法
    // ========================================================================

    /// @brief 创建新的 RequestContext，自动生成 requestId
    static RequestContext create(const QString& sourceComponent = QString());

    /// @brief 从已有 traceId 创建 (跨服务传递)
    static RequestContext fromTrace(const QString& traceId,
                                     const QString& sourceComponent = QString());

    /// @brief 生成新的 spanId (子 Span)
    static QString generateSpanId();

    // ========================================================================
    // 便捷方法
    // ========================================================================

    /// @brief 请求已耗时 (毫秒)
    int64_t elapsedMs() const;

    /// @brief 添加元数据
    RequestContext& withMeta(const QString& key, const QString& value) {
        metadata.insert(key, value);
        return *this;
    }

    /// @brief 获取元数据
    QString meta(const QString& key) const {
        return metadata.value(key);
    }
};

// ============================================================================
// RequestContextGuard — RAII 作用域守卫
// ============================================================================
//
// 在构造时将 Context 推入 thread_local 栈，析构时弹出。
// 支持嵌套 (子 Span 场景)。
//
// 用法:
//   {
//       RequestContextGuard guard(ctx);
//       // 此作用域内 RequestContext::current() 可用
//       auto& current = RequestContext::current();
//   }
//   // 离开作用域后自动清理
//
// @thread_safety thread_local 隔离，无跨线程问题

class RequestContextGuard {
public:
    explicit RequestContextGuard(const RequestContext& ctx);
    ~RequestContextGuard();

    RequestContextGuard(const RequestContextGuard&) = delete;
    RequestContextGuard& operator=(const RequestContextGuard&) = delete;

    /// @brief 获取当前作用域的 Context (只读引用)
    static const RequestContext& current();

    /// @brief 是否有活跃的 Context
    static bool hasCurrent();

    /// @brief 安全获取 requestId (无 Context 时返回空字符串)
    static QString currentRequestId();

    /// @brief 安全获取 traceId
    static QString currentTraceId();

private:
    bool m_popped = false;
};

} // namespace sc

#endif // SOUL_CORE_REQUEST_CONTEXT_H
