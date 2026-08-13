// ============================================================================
// inmemory_message_bus.cpp — InMemoryMessageBus 实现 [v2.9.2 增强]
// ============================================================================

#include "soul/event/inmemory_message_bus.h"
#include "soul/core/request_context.h"
#include "soul/core/uuid.h"
#include <algorithm>

namespace sc {

// ============================================================================
// Message 实现
// ============================================================================

Message Message::create(const QString& topic, const QByteArray& payload) {
    Message msg;
    msg.id = QString::fromStdString(Uuid::generate());
    msg.topic = topic;
    msg.payload = payload;
    msg.contentType = "application/octet-stream";
    msg.timestamp = QDateTime::currentDateTimeUtc();
    return msg;
}

Message Message::fromJson(const QString& topic, const QString& jsonBody) {
    Message msg = create(topic, jsonBody.toUtf8());
    msg.contentType = "application/json";
    return msg;
}

Message& Message::inheritContext() {
    if (RequestContextGuard::hasCurrent()) {
        auto& ctx = RequestContextGuard::current();
        traceId = ctx.traceId;
        correlationId = ctx.requestId;
        sourceComponent = ctx.sourceComponent;
    }
    return *this;
}

// ============================================================================
// InMemoryMessageBus 实现
// ============================================================================

InMemoryMessageBus::InMemoryMessageBus() = default;

InMemoryMessageBus::~InMemoryMessageBus() {
    shutdown();
}

void InMemoryMessageBus::insertSorted(QVector<SubscriberEntry>& list,
                                       SubscriberEntry entry) {
    auto it = std::lower_bound(list.begin(), list.end(), entry.priority,
        [](const SubscriberEntry& a, EventPriority p) {
            return static_cast<int>(a.priority) > static_cast<int>(p);
        });
    list.insert(it, std::move(entry));
}

// v2.9.2: 快照 + 自动清理已过期 Subscription
QVector<InMemoryMessageBus::SubscriberEntry>
InMemoryMessageBus::snapshotAndClean(const QString& channel) {
    std::unique_lock lock(m_mutex);
    auto it = m_subscribers.find(channel);
    if (it == m_subscribers.end()) {
        return {};
    }

    auto& list = it.value();
    QVector<SubscriberEntry> active;
    int expiredCount = 0;

    for (auto& entry : list) {
        if (!entry.subscription.expired()) {
            active.push_back(entry);
        } else {
            expiredCount++;
        }
    }

    // 更新列表: 仅保留活跃订阅者
    if (expiredCount > 0) {
        list = active;
        m_totalSubs.fetch_sub(expiredCount, std::memory_order_relaxed);
    }

    return active;
}

// ============================================================================
// publish
// ============================================================================

void InMemoryMessageBus::publish(const QString& channel, const Message& message) {
    if (m_shutdown.load(std::memory_order_acquire)) {
        return;
    }

    auto entries = snapshotAndClean(channel);
    if (entries.empty()) {
        return;
    }

    auto msgPtr = std::make_shared<Message>(message);

    for (auto& entry : entries) {
        auto sub = entry.subscription.lock();
        if (!sub || !sub->isValid()) {
            continue;
        }

        try {
            sub->invoke(msgPtr);
        } catch (...) {
            sub->invalidate();
        }
    }
}

void InMemoryMessageBus::publish(const QString& channel,
                                  const std::shared_ptr<void>& message) {
    if (m_shutdown.load(std::memory_order_acquire)) {
        return;
    }

    auto entries = snapshotAndClean(channel);
    for (auto& entry : entries) {
        auto sub = entry.subscription.lock();
        if (!sub || !sub->isValid()) {
            continue;
        }

        try {
            sub->invoke(message);
        } catch (...) {
            sub->invalidate();
        }
    }
}

// ============================================================================
// subscribe
// ============================================================================

MessageSubscriptionPtr InMemoryMessageBus::subscribe(
    const QString& channel,
    MessageSubscription::Callback callback) {
    return subscribe(channel, EventPriority::Normal, std::move(callback));
}

MessageSubscriptionPtr InMemoryMessageBus::subscribe(
    const QString& channel,
    EventPriority priority,
    MessageSubscription::Callback callback) {

    if (m_shutdown.load(std::memory_order_acquire)) {
        return nullptr;
    }

    // v2.9.2: 传入 weak_ptr<IMessageBus>，析构时自动取消订阅
    // 但核心清理依赖 InMemoryMessageBus 内部的 weak_ptr 自动感知
    auto sub = std::make_shared<MessageSubscription>(
        channel, priority, std::move(callback), weak_from_this());

    {
        std::unique_lock lock(m_mutex);
        SubscriberEntry entry{std::weak_ptr<MessageSubscription>(sub), priority};
        auto& list = m_subscribers[channel];
        insertSorted(list, std::move(entry));
        m_totalSubs.fetch_add(1, std::memory_order_relaxed);
    }

    return sub;
}

// ============================================================================
// unsubscribe
// ============================================================================

void InMemoryMessageBus::unsubscribe(MessageSubscriptionPtr subscription) {
    if (!subscription) return;

    subscription->invalidate();

    std::unique_lock lock(m_mutex);
    for (auto it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
        auto& list = it.value();
        auto rit = std::remove_if(list.begin(), list.end(),
            [&subscription](const SubscriberEntry& entry) {
                auto locked = entry.subscription.lock();
                return !locked || locked == subscription;
            });
        if (rit != list.end()) {
            int removed = static_cast<int>(list.end() - rit);
            list.erase(rit, list.end());
            m_totalSubs.fetch_sub(removed, std::memory_order_relaxed);
        }
    }
}

void InMemoryMessageBus::unsubscribeAll(const QString& channel) {
    std::unique_lock lock(m_mutex);
    auto it = m_subscribers.find(channel);
    if (it != m_subscribers.end()) {
        int count = static_cast<int>(it.value().size());
        m_totalSubs.fetch_sub(count, std::memory_order_relaxed);
        m_subscribers.erase(it);
    }
}

// ============================================================================
// 查询
// ============================================================================

bool InMemoryMessageBus::hasSubscribers(const QString& channel) const {
    std::shared_lock lock(m_mutex);
    auto it = m_subscribers.find(channel);
    if (it == m_subscribers.end()) return false;

    // 检查是否有至少一个活跃订阅者
    for (auto& entry : it.value()) {
        if (!entry.subscription.expired()) return true;
    }
    return false;
}

int InMemoryMessageBus::subscriberCount(const QString& channel) const {
    std::shared_lock lock(m_mutex);
    auto it = m_subscribers.find(channel);
    if (it == m_subscribers.end()) return 0;

    int count = 0;
    for (auto& entry : it.value()) {
        if (!entry.subscription.expired()) count++;
    }
    return count;
}

int InMemoryMessageBus::totalSubscriberCount() const {
    return m_totalSubs.load(std::memory_order_relaxed);
}

// ============================================================================
// 生命周期
// ============================================================================

void InMemoryMessageBus::shutdown() {
    m_shutdown.store(true, std::memory_order_release);

    std::unique_lock lock(m_mutex);
    m_subscribers.clear();
    m_totalSubs.store(0, std::memory_order_relaxed);
}

bool InMemoryMessageBus::isShutdown() const {
    return m_shutdown.load(std::memory_order_acquire);
}

} // namespace sc
