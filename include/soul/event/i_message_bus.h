#ifndef SOUL_EVENT_I_MESSAGE_BUS_H
#define SOUL_EVENT_I_MESSAGE_BUS_H

#include <QString>
#include <memory>
#include "subscription.h"

namespace sc {

/**
 * @class IMessageBus
 * @brief 消息总线接口定义
 *
 * IMessageBus 定义了发布-订阅模式的消息总线接口，支持：
 * - 订阅指定频道的消息
 * - 发布消息到指定频道
 * - 取消订阅
 * - 查询订阅者数量
 *
 * 使用方式：
 * @code
 * std::shared_ptr<IMessageBus> bus = std::make_shared<MessageBus>();
 * auto subscription = bus->subscribe("channel", [](const std::shared_ptr<void>& msg) {
 *     // 处理消息
 * });
 * bus->publish("channel", std::make_shared<MyMessage>());
 * @endcode
 *
 * @see MessageSubscription, EventPriority
 */
class IMessageBus {
public:
    virtual ~IMessageBus() = default;

    virtual MessageSubscriptionPtr subscribe(const QString& channel, MessageSubscription::Callback callback) = 0;
    virtual MessageSubscriptionPtr subscribe(const QString& channel, EventPriority priority, MessageSubscription::Callback callback) = 0;
    virtual void publish(const QString& channel, const std::shared_ptr<void>& message) = 0;
    virtual void unsubscribe(MessageSubscriptionPtr subscription) = 0;
    virtual void unsubscribeAll(const QString& channel) = 0;
    virtual bool hasSubscribers(const QString& channel) const = 0;
    virtual int subscriberCount(const QString& channel) const = 0;
};

using MessageBusPtr = std::shared_ptr<IMessageBus>;

}

#endif
