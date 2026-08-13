#ifndef SOUL_EVENT_INMEMORY_MESSAGE_BUS_H
#define SOUL_EVENT_INMEMORY_MESSAGE_BUS_H

// ============================================================================
// inmemory_message_bus.h — InMemory MessageBus 实现 [v2.9.1 新增 / v2.9.2 增强]
// ============================================================================
//
// v2.9.2 变更:
//   - SubscriberEntry 使用 weak_ptr<MessageSubscription> 存储
//   - publish 时自动清理已析构的 Subscription
//   - Subscription 析构 → RAII 自动取消订阅 (不再需要手动 unsubscribe)
//   - 防止 Bus ↔ Subscription 循环引用
//
// 设计:
//   - 纯进程内实现，不依赖任何外部 Broker
//   - 同步 publish: Consumer 在 publish() 线程中执行
//   - 线程安全: 读写锁保护订阅列表
//   - 所有权: subscribe() 返回 Subscription，析构自动取消
//
// Delivery Semantics: At-most-once (进程内 best-effort)
// Ordering: 同一频道 + 同一 publish 线程 → 有序
// Backpressure: 同步模式，Consumer 慢会阻塞 Producer

#include "soul/event/i_message_bus.h"
#include <QString>
#include <QHash>
#include <QVector>
#include <shared_mutex>
#include <memory>
#include <atomic>

namespace sc {

class InMemoryMessageBus : public IMessageBus,
                           public std::enable_shared_from_this<InMemoryMessageBus> {
public:
    InMemoryMessageBus();
    ~InMemoryMessageBus() override;

    // === IMessageBus ===

    void publish(const QString& channel, const Message& message) override;
    void publish(const QString& channel, const std::shared_ptr<void>& message) override;

    MessageSubscriptionPtr subscribe(const QString& channel,
                                      MessageSubscription::Callback callback) override;

    MessageSubscriptionPtr subscribe(const QString& channel,
                                      EventPriority priority,
                                      MessageSubscription::Callback callback) override;

    void unsubscribe(MessageSubscriptionPtr subscription) override;
    void unsubscribeAll(const QString& channel) override;

    bool hasSubscribers(const QString& channel) const override;
    int subscriberCount(const QString& channel) const override;
    int totalSubscriberCount() const override;

    // === 生命周期 ===

    void shutdown();
    bool isShutdown() const;

private:
    // v2.9.2: 使用 weak_ptr 存储，自动感知 Subscription 析构
    struct SubscriberEntry {
        std::weak_ptr<MessageSubscription> subscription;
        EventPriority priority = EventPriority::Normal;
    };

    void insertSorted(QVector<SubscriberEntry>& list, SubscriberEntry entry);

    // 快照模式: 复制活跃订阅者列表，释放锁后回调
    // 自动清理已过期 (析构) 的 Subscription
    QVector<SubscriberEntry> snapshotAndClean(const QString& channel);

    mutable std::shared_mutex m_mutex;
    QHash<QString, QVector<SubscriberEntry>> m_subscribers;
    std::atomic<bool> m_shutdown{false};
    std::atomic<int> m_totalSubs{0};
};

} // namespace sc

#endif // SOUL_EVENT_INMEMORY_MESSAGE_BUS_H
