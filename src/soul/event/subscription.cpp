// ============================================================================
// subscription.cpp — Subscription 实现 [v2.9.2 增强]
// ============================================================================

#include "soul/event/subscription.h"

namespace sc {

MessageSubscription::MessageSubscription(const QString& eventType,
                                          EventPriority priority,
                                          Callback callback,
                                          std::weak_ptr<IMessageBus> bus)
    : m_eventType(eventType)
    , m_priority(priority)
    , m_callback(std::move(callback))
    , m_valid(true)
    , m_bus(std::move(bus))
{}

MessageSubscription::~MessageSubscription() {
    // v2.9.2: RAII 自动取消订阅
    // Bus 内部使用 weak_ptr<MessageSubscription> 存储，
    // 当此对象析构时，Bus 的 weak_ptr 自动过期。
    // 下一次 publish() 调用 snapshotAndClean() 时会自动清理。
    // 无需主动调用 unsubscribe()。
    m_valid = false;
}

void MessageSubscription::invoke(const std::shared_ptr<void>& message) {
    if (m_valid && m_callback) {
        m_callback(message);
    }
}

} // namespace sc
