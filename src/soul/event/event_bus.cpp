#include "soul/event/event_bus.h"
#include <algorithm>

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

Subscription DefaultEventBus::subscribe(const std::string& topic, const Handler& handler) {
    auto handlerPtr = std::make_shared<Handler>(handler);
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_handlers[topic].push_back(handlerPtr);
    }

    return Subscription([this, topic, handlerPtr]() {
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

size_t DefaultEventBus::subscriberCount(const std::string& topic) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_handlers.find(topic);
    if (it != m_handlers.end()) {
        return it->second.size();
    }
    return 0;
}

}