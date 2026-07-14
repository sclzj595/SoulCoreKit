#ifndef SOUL_EVENT_SUBSCRIPTION_H
#define SOUL_EVENT_SUBSCRIPTION_H

#include <QObject>
#include <functional>
#include "event_priority.h"

namespace sc {

/**
 * @class Subscription
 * @brief 消息订阅封装类
 *
 * Subscription 封装了一次消息订阅，包含事件类型、优先级和回调函数。
 * 支持验证订阅有效性和失效处理。
 *
 * @see IMessageBus, EventPriority
 */
class Subscription {
public:
    /**
     * @brief 消息回调函数类型
     * @param message 消息数据（任意类型）
     */
    using Callback = std::function<void(const std::shared_ptr<void>&)>;

    /**
     * @brief 构造函数
     * @param eventType 事件类型
     * @param priority 订阅优先级
     * @param callback 消息回调函数
     */
    Subscription(const QString& eventType, EventPriority priority, Callback callback);

    /**
     * @brief 获取事件类型
     * @return 事件类型
     */
    QString eventType() const;

    /**
     * @brief 获取订阅优先级
     * @return 优先级
     */
    EventPriority priority() const;

    /**
     * @brief 获取回调函数
     * @return 回调函数
     */
    Callback callback() const;

    /**
     * @brief 检查订阅是否有效
     * @return 如果有效返回 true
     */
    bool isValid() const;

    /**
     * @brief 使订阅失效
     */
    void invalidate();

private:
    QString m_eventType;
    EventPriority m_priority;
    Callback m_callback;
    bool m_valid;
};

/**
 * @brief 订阅智能指针类型
 */
using SubscriptionPtr = std::shared_ptr<Subscription>;

}

#endif
