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
 * @see Subscription, EventPriority
 */
class IMessageBus {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IMessageBus() = default;

    /**
     * @brief 订阅指定频道（默认优先级）
     * @param channel 频道名称
     * @param callback 消息回调函数
     * @return 订阅对象
     */
    virtual SubscriptionPtr subscribe(const QString& channel, Subscription::Callback callback) = 0;

    /**
     * @brief 订阅指定频道（指定优先级）
     * @param channel 频道名称
     * @param priority 订阅优先级
     * @param callback 消息回调函数
     * @return 订阅对象
     */
    virtual SubscriptionPtr subscribe(const QString& channel, EventPriority priority, Subscription::Callback callback) = 0;

    /**
     * @brief 发布消息到指定频道
     * @param channel 频道名称
     * @param message 消息数据（任意类型）
     */
    virtual void publish(const QString& channel, const std::shared_ptr<void>& message) = 0;

    /**
     * @brief 取消指定订阅
     * @param subscription 订阅对象
     */
    virtual void unsubscribe(SubscriptionPtr subscription) = 0;

    /**
     * @brief 取消指定频道的所有订阅
     * @param channel 频道名称
     */
    virtual void unsubscribeAll(const QString& channel) = 0;

    /**
     * @brief 检查指定频道是否有订阅者
     * @param channel 频道名称
     * @return 如果有订阅者返回 true
     */
    virtual bool hasSubscribers(const QString& channel) const = 0;

    /**
     * @brief 获取指定频道的订阅者数量
     * @param channel 频道名称
     * @return 订阅者数量
     */
    virtual int subscriberCount(const QString& channel) const = 0;
};

/**
 * @brief 消息总线智能指针类型
 */
using MessageBusPtr = std::shared_ptr<IMessageBus>;

}

#endif
