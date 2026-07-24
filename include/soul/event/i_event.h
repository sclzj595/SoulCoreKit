#ifndef SOUL_EVENT_IEVENT_H
#define SOUL_EVENT_IEVENT_H

#include <string>

namespace sc {

/**
 * @class IEvent
 * @brief 事件接口定义
 *
 * IEvent 定义了事件的标准接口，所有自定义事件都应实现此接口。
 * 事件包含：
 * - topic：事件主题，用于事件分类和路由
 * - data：事件数据，通常是序列化后的字符串
 *
 * 使用方式：
 * @code
 * class MyEvent : public IEvent {
 * public:
 *     std::string topic() const override { return "my_topic"; }
 *     std::string data() const override { return "event_data"; }
 * };
 * @endcode
 *
 * @see EventBus
 */
class IEvent {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IEvent() = default;

    /**
     * @brief 获取事件主题
     * @return 事件主题
     */
    virtual std::string topic() const = 0;

    /**
     * @brief 获取事件数据
     * @return 事件数据（序列化字符串）
     */
    virtual std::string data() const = 0;
};

}

#endif
