#include "soul/event/event_bus.h"
#include <memory>
#include <mutex>
#include <algorithm>
#include <string>
#include "soul/async/thread_pool.h"

namespace sc {

Subscription::Subscription(std::function<void()> unsubscribeFn)
    : m_unsubscribeFn(std::move(unsubscribeFn)) {
}

Subscription::~Subscription() {
    unsubscribe();
}

Subscription::Subscription(Subscription&& other) noexcept
    : m_unsubscribeFn(std::move(other.m_unsubscribeFn)),
      m_valid(other.m_valid.load()) {
    other.m_valid.store(false);
}

Subscription& Subscription::operator=(Subscription&& other) noexcept {
    if (this != &other) {
        unsubscribe();
        m_unsubscribeFn = std::move(other.m_unsubscribeFn);
        m_valid.store(other.m_valid.load());
        other.m_valid.store(false);
    }
    return *this;
}

void Subscription::unsubscribe() {
    if (m_valid.exchange(false) && m_unsubscribeFn) {
        m_unsubscribeFn();
    }
}

bool Subscription::isValid() const {
    return m_valid.load();
}

DefaultEventBus::DefaultEventBus() {
}

DefaultEventBus::~DefaultEventBus() {
    // [审计] 置活跃标志为 false, 使所有已订阅的 Subscription 在析构时不再触碰本对象。
    m_alive->store(false, std::memory_order_release);
}

Subscription DefaultEventBus::subscribe(const std::string& topic, const Handler& handler) {
    auto handlerPtr = std::make_shared<Handler>(handler);
    std::weak_ptr<std::atomic<bool>> aliveWeak = m_alive;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_handlers[topic].push_back(handlerPtr);
    }

    return Subscription([this, aliveWeak, topic, handlerPtr]() {
        // 总线可能已析构: 用弱引用探测存活状态, 避免 UAF。
        // alive==true 仅当总线仍存活(总线析构时先置 false 再释放成员), 故此时访问
        // this 的 m_mutex/m_handlers 是安全的; alive 失效(弱引用为空)同样安全跳过。
        auto alive = aliveWeak.lock();
        if (!alive || !alive->load(std::memory_order_acquire)) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlers.find(topic);
        if (it != m_handlers.end()) {
            auto& handlers = it->second;
            handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                [handlerPtr](const std::shared_ptr<Handler>& h) { return h == handlerPtr; }), handlers.end());
        }
    });
}

void DefaultEventBus::publish(const std::string& topic, const std::string& data) {
    std::vector<std::shared_ptr<Handler>> handlersCopy;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlers.find(topic);
        if (it != m_handlers.end()) {
            handlersCopy = it->second;
        }
    }

    for (const auto& handlerPtr : handlersCopy) {
        (*handlerPtr)(data);
    }
}

void DefaultEventBus::publishAsync(const std::string& topic, const std::string& data) {
    std::vector<std::shared_ptr<Handler>> handlersCopy;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlers.find(topic);
        if (it != m_handlers.end()) {
            handlersCopy = it->second;
        }
    }
    
    ThreadPool::instance().start([handlersCopy, data]() {
        for (const auto& handlerPtr : handlersCopy) {
            (*handlerPtr)(data);
        }
    });
}

size_t DefaultEventBus::subscriberCount(const std::string& topic) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_handlers.find(topic);
    if (it != m_handlers.end()) {
        return it->second.size();
    }
    return 0;
}

}