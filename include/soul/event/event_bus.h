#ifndef SOUL_EVENT_EVENT_BUS_H
#define SOUL_EVENT_EVENT_BUS_H

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include "soul/core/interface.h"

namespace sc {

/**
 * @class Subscription
 * @brief 订阅封装类，管理订阅生命周期
 *
 * Subscription 封装了一次订阅操作，提供取消订阅和验证功能。
 * 当 Subscription 对象被销毁时会自动取消订阅。
 *
 * @see IEventBus, DefaultEventBus
 */
class Subscription {
public:
    /**
     * @brief 默认构造函数
     */
    Subscription() = default;

    /**
     * @brief 构造函数
     * @param unsubscribeFn 取消订阅的函数
     */
    Subscription(std::function<void()> unsubscribeFn);

    /**
     * @brief 析构函数（自动取消订阅）
     */
    ~Subscription();

    /**
     * @brief 禁止拷贝构造
     */
    Subscription(const Subscription&) = delete;

    /**
     * @brief 禁止拷贝赋值
     */
    Subscription& operator=(const Subscription&) = delete;

    /**
     * @brief 移动构造函数
     */
    Subscription(Subscription&&) noexcept;

    /**
     * @brief 移动赋值运算符
     */
    Subscription& operator=(Subscription&&) noexcept;

    /**
     * @brief 取消订阅
     */
    void unsubscribe();

    /**
     * @brief 检查订阅是否有效
     * @return 如果有效返回 true
     */
    bool isValid() const;

private:
    std::function<void()> m_unsubscribeFn;
    std::atomic<bool> m_valid{true};
};

/**
 * @class IEventBus
 * @brief 事件总线接口定义
 *
 * IEventBus 定义了事件发布-订阅的标准接口，所有事件总线实现都应继承此接口。
 * 支持主题分类和订阅管理。
 *
 * @see Subscription, DefaultEventBus
 */
class IEventBus : public IInterface {
public:
    /**
     * @brief 事件处理函数类型
     * @param data 事件数据（字符串格式）
     */
    using Handler = std::function<void(const std::string&)>;

    /**
     * @brief 虚析构函数
     */
    ~IEventBus() override = default;

    /**
     * @brief 订阅指定主题
     * @param topic 事件主题
     * @param handler 事件处理函数
     * @return 订阅对象
     */
    virtual Subscription subscribe(const std::string& topic, const Handler& handler) = 0;

    /**
     * @brief 发布事件到指定主题
     * @param topic 事件主题
     * @param data 事件数据
     */
    virtual void publish(const std::string& topic, const std::string& data) = 0;

    /**
     * @brief 异步发布事件到指定主题（在线程池中执行）
     *
     * 与 publish() 不同，publishAsync() 会立即返回，事件处理函数
     * 将在 ThreadPool 的工作线程中异步执行，不会阻塞调用线程。
     *
     * 注意：事件回调将在线程池线程中执行，而不是 GUI 线程。
     * 如果需要在 GUI 线程中处理事件，请在回调中使用 Dispatcher。
     *
     * @param topic 事件主题
     * @param data 事件数据
     *
     * @code
     * // 在 worker 线程中发布事件
     * eventBus->publishAsync("data.loaded", data);
     *
     * // 如果需要在 GUI 线程处理
     * eventBus->subscribe("data.loaded", [](const std::string& data) {
     *     Dispatcher::invoke([data]() {
     *         // 在 GUI 线程中更新 UI
     *     });
     * });
     * @endcode
     */
    virtual void publishAsync(const std::string& topic, const std::string& data) = 0;

    virtual size_t subscriberCount(const std::string& topic) const = 0;
};

/**
 * @class DefaultEventBus
 * @brief 默认事件总线实现
 *
 * DefaultEventBus 是 IEventBus 的默认实现，支持线程安全的事件发布和订阅。
 * 使用 std::unordered_map 存储主题到处理函数的映射，使用 std::mutex 保证线程安全。
 *
 * 使用方式：
 * @code
 * DefaultEventBus bus;
 * auto subscription = bus.subscribe("user.login", [](const std::string& data) {
 *     // 处理登录事件
 * });
 * bus.publish("user.login", "{\"user_id\": \"123\"}");
 * @endcode
 *
 * @see IEventBus, Subscription
 */
class DefaultEventBus : public IEventBus {
public:
    /**
     * @brief 构造函数
     */
    DefaultEventBus();

    /**
     * @brief 析构函数
     */
    ~DefaultEventBus() override = default;

    /**
     * @brief 禁止拷贝构造
     */
    DefaultEventBus(const DefaultEventBus&) = delete;

    /**
     * @brief 禁止拷贝赋值
     */
    DefaultEventBus& operator=(const DefaultEventBus&) = delete;

    /**
     * @brief 订阅指定主题
     * @param topic 事件主题
     * @param handler 事件处理函数
     * @return 订阅对象
     */
    Subscription subscribe(const std::string& topic, const Handler& handler) override;

    /**
     * @brief 发布事件到指定主题
     * @param topic 事件主题
     * @param data 事件数据
     */
    void publish(const std::string& topic, const std::string& data) override;

    void publishAsync(const std::string& topic, const std::string& data) override;

    size_t subscriberCount(const std::string& topic) const override;

    std::string interfaceName() const override { return "IEventBus"; }

private:
    std::unordered_map<std::string, std::vector<std::shared_ptr<Handler>>> m_handlers;
    mutable std::mutex m_mutex;
};

}

#endif
