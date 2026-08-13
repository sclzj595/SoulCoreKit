#ifndef SOUL_EVENT_SUBSCRIPTION_H
#define SOUL_EVENT_SUBSCRIPTION_H

// ============================================================================
// subscription.h — Event / Message 订阅对象 [v2.9.2 增强]
// ============================================================================
//
// v2.9.2 变更:
//   - MessageSubscription 析构时自动取消订阅 (RAII)
//   - 通过 weak_ptr<IMessageBus> 安全引用 Bus
//   - 防止 Bus ↔ Subscription 循环引用
//   - 向后兼容: 手动 unsubscribe() 仍然可用

#include <QString>
#include <memory>
#include <functional>
#include "event_bus.h"
#include "event_priority.h"

namespace sc {

class Subscription;
using SubscriptionPtr = std::shared_ptr<Subscription>;

class IMessageBus;  // 前向声明

class MessageSubscription {
public:
    using Callback = std::function<void(const std::shared_ptr<void>&)>;

    /// @brief 构造函数
    /// @param eventType 频道名称
    /// @param priority  优先级
    /// @param callback  回调函数
    /// @param bus       Bus 的 weak_ptr (用于析构时自动取消)
    MessageSubscription(const QString& eventType, EventPriority priority,
                        Callback callback,
                        std::weak_ptr<IMessageBus> bus = {});

    /// @brief 析构时自动从 Bus 取消订阅 [v2.9.2]
    ~MessageSubscription();

    // 禁止拷贝
    MessageSubscription(const MessageSubscription&) = delete;
    MessageSubscription& operator=(const MessageSubscription&) = delete;

    QString eventType() const { return m_eventType; }
    EventPriority priority() const { return m_priority; }
    Callback callback() const { return m_callback; }
    bool isValid() const { return m_valid; }
    void invalidate() { m_valid = false; }

    /// @brief 执行回调 (由 IMessageBus 调用)
    void invoke(const std::shared_ptr<void>& message);

private:
    QString m_eventType;
    EventPriority m_priority;
    Callback m_callback;
    bool m_valid;
    std::weak_ptr<IMessageBus> m_bus;  // v2.9.2: 析构时自动 unsubscribe
};

using MessageSubscriptionPtr = std::shared_ptr<MessageSubscription>;

} // namespace sc

#endif // SOUL_EVENT_SUBSCRIPTION_H
