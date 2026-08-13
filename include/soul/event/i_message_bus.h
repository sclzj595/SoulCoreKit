#ifndef SOUL_EVENT_I_MESSAGE_BUS_H
#define SOUL_EVENT_I_MESSAGE_BUS_H

// ============================================================================
// i_message_bus.h — 统一消息总线抽象 [v2.9.1 增强]
// ============================================================================
//
// IMessageBus 定义进程内发布-订阅模式的消息总线接口。
// 它与现有的 IEventBus/TypedEventBus 的区别:
//
//   IEventBus / TypedEventBus  → 进程内事件分发 (Event = in-process signal)
//   IMessageBus                → 消息生产/消费抽象 (Message = payload delivery)
//
// v2.9.1 新增:
//   - InMemoryMessageBus (完整实现)
//   - Message 模型 (id/topic/payload/headers/timestamp/traceId)
//   - 明确所有权: subscribe() 返回 Subscription，销毁自动取消
//   - 线程安全保证
//
// 与 MQ 模块的关系:
//   - MQ 模块 (IMQProducer/IMQConsumer/IAmqpBackend) 是 AMQP 体系
//   - IMessageBus 是更通用的消息抽象，可用于进程内或桥接到外部 Broker
//
// 用法:
//   auto bus = std::make_shared<InMemoryMessageBus>();
//   auto sub = bus->subscribe("user.created", [](const Message& msg) {
//       SC_INFO("Received: " + msg.topic);
//   });
//   bus->publish(Message::create("user.created", R"({"id":42})"));
//   // sub 析构时自动取消订阅

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QVariant>
#include <memory>
#include <functional>
#include <string>
#include <atomic>
#include "subscription.h"

namespace sc {

// ============================================================================
// Message — 消息模型 [v2.9.1 新增]
// ============================================================================

struct Message {
    // --- 核心字段 ---
    QString id;         // 消息唯一 ID
    QString topic;      // 消息主题 (如 "user.created")
    QByteArray payload; // 消息体 (字节，与 Transport 解耦)

    // --- 元数据 ---
    QString contentType;  // "application/json", "application/octet-stream"
    QHash<QString, QString> headers;

    // --- 追踪 ---
    QString traceId;
    QString correlationId;
    QString sourceComponent;  // "http-server", "rpc-client"

    // --- 时间 ---
    QDateTime timestamp;

    // ========================================================================
    // 工厂方法
    // ========================================================================

    /// @brief 创建消息 (自动生成 ID + 时间戳)
    static Message create(const QString& topic, const QByteArray& payload = QByteArray());

    /// @brief 从 JSON 字符串创建
    static Message fromJson(const QString& topic, const QString& jsonBody);

    /// @brief 从 RequestContext 继承追踪信息
    Message& inheritContext();

    // ========================================================================
    // 便捷方法
    // ========================================================================

    /// @brief 添加 Header
    Message& withHeader(const QString& key, const QString& value) {
        headers.insert(key, value);
        return *this;
    }

    /// @brief 设置 Content-Type
    Message& withContentType(const QString& ct) {
        contentType = ct;
        return *this;
    }

    /// @brief Payload 转为字符串 (仅 text/json)
    QString payloadString() const { return QString::fromUtf8(payload); }
};

// ============================================================================
// IMessageBus — 消息总线接口 [v2.9.1 增强]
// ============================================================================

class IMessageBus {
public:
    virtual ~IMessageBus() = default;

    // === 发布 ===

    /// @brief 发布消息到指定频道 (同步，调用方线程执行 Consumer)
    virtual void publish(const QString& channel, const Message& message) = 0;

    /// @brief 发布消息 (兼容旧 API: std::shared_ptr<void>)
    virtual void publish(const QString& channel, const std::shared_ptr<void>& message) = 0;

    // === 订阅 ===

    /// @brief 订阅频道 (默认优先级 Normal)
    /// @return Subscription — 销毁时自动取消订阅
    virtual MessageSubscriptionPtr subscribe(const QString& channel,
                                              MessageSubscription::Callback callback) = 0;

    /// @brief 订阅频道 (带优先级)
    virtual MessageSubscriptionPtr subscribe(const QString& channel,
                                              EventPriority priority,
                                              MessageSubscription::Callback callback) = 0;

    // === 取消订阅 ===

    /// @brief 取消指定订阅
    virtual void unsubscribe(MessageSubscriptionPtr subscription) = 0;

    /// @brief 取消频道所有订阅
    virtual void unsubscribeAll(const QString& channel) = 0;

    // === 查询 ===

    /// @brief 频道是否有订阅者
    virtual bool hasSubscribers(const QString& channel) const = 0;

    /// @brief 频道订阅者数量
    virtual int subscriberCount(const QString& channel) const = 0;

    /// @brief 总订阅者数量
    virtual int totalSubscriberCount() const = 0;
};

using MessageBusPtr = std::shared_ptr<IMessageBus>;

} // namespace sc

#endif // SOUL_EVENT_I_MESSAGE_BUS_H
