#ifndef SOUL_EVENT_TYPED_EVENT_BUS_H
#define SOUL_EVENT_TYPED_EVENT_BUS_H

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <any>
#include <typeindex>
#include <utility>
#include <atomic>
#include "soul/event/event_bus.h"
#include "soul/async/thread_pool.h"
#include "soul/logging/logger.h"

namespace sc {

enum class DispatchPolicy {
    Sync,
    Async,
};

namespace detail {

struct TypeTopicHash {
    size_t operator()(const std::pair<std::type_index, std::string>& key) const {
        size_t h1 = std::hash<std::type_index>()(key.first);
        size_t h2 = std::hash<std::string>()(key.second);
        return h1 ^ (h2 << 1);
    }
};

struct TypeTopicEqual {
    bool operator()(const std::pair<std::type_index, std::string>& a,
                    const std::pair<std::type_index, std::string>& b) const {
        return a.first == b.first && a.second == b.second;
    }
};

}

class TypedEventBus : public std::enable_shared_from_this<TypedEventBus> {
public:
    using HandlerBase = std::function<void(const std::any&)>;

    static std::shared_ptr<TypedEventBus> create() {
        return std::shared_ptr<TypedEventBus>(new TypedEventBus());
    }

    ~TypedEventBus() = default;

    TypedEventBus(const TypedEventBus&) = delete;
    TypedEventBus& operator=(const TypedEventBus&) = delete;

    template<typename T>
    using Handler = std::function<void(const T&)>;

    template<typename T>
    Subscription subscribe(const std::string& topic, Handler<T> handler,
                          DispatchPolicy policy = DispatchPolicy::Sync) {
        std::pair<std::type_index, std::string> key{std::type_index(typeid(T)), topic};

        auto typedHandler = std::make_shared<Handler<T>>(std::move(handler));

        auto wrapper = [typedHandler, policy](const std::any& data) {
            try {
                const T& value = std::any_cast<const T&>(data);

                if (policy == DispatchPolicy::Async) {
                    T valueCopy = value;
                    ThreadPool::instance().start([typedHandler, valueCopy]() {
                        try {
                            (*typedHandler)(valueCopy);
                        } catch (const std::exception& e) {
                            Logger::instance().error("Async handler exception: " + std::string(e.what()), "EventBus");
                        } catch (...) {
                            Logger::instance().error("Async handler unknown exception", "EventBus");
                        }
                    });
                } else {
                    (*typedHandler)(value);
                }
            } catch (const std::bad_any_cast&) {
                Logger::instance().error("Type mismatch in handler", "EventBus");
            } catch (const std::exception& e) {
                Logger::instance().error("Handler exception: " + std::string(e.what()), "EventBus");
            } catch (...) {
                Logger::instance().error("Handler unknown exception", "EventBus");
            }
        };

        auto wrapperPtr = std::make_shared<std::function<void(const std::any&)>>(std::move(wrapper));
        auto weakThis = weak_from_this();

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_handlers[key].push_back(wrapperPtr);
        }

        return Subscription([weakThis, key, wrapperPtr]() {
            auto sharedThis = weakThis.lock();
            if (!sharedThis) {
                return;
            }
            std::lock_guard<std::mutex> lock(sharedThis->m_mutex);
            auto it = sharedThis->m_handlers.find(key);
            if (it != sharedThis->m_handlers.end()) {
                auto& handlers = it->second;
                auto found = std::find(handlers.begin(), handlers.end(), wrapperPtr);
                if (found != handlers.end()) {
                    handlers.erase(found);
                }
                if (handlers.empty()) {
                    sharedThis->m_handlers.erase(it);
                }
            }
        });
    }

    template<typename T>
    void publish(const std::string& topic, const T& data) {
        std::pair<std::type_index, std::string> key{std::type_index(typeid(T)), topic};
        std::vector<std::shared_ptr<std::function<void(const std::any&)>>> handlersCopy;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_handlers.find(key);
            if (it != m_handlers.end()) {
                handlersCopy = it->second;
            }
        }

        std::any anyData = data;
        for (const auto& handlerPtr : handlersCopy) {
            (*handlerPtr)(anyData);
        }
    }

    template<typename T>
    size_t subscriberCount(const std::string& topic) const {
        std::pair<std::type_index, std::string> key{std::type_index(typeid(T)), topic};
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlers.find(key);
        if (it != m_handlers.end()) {
            return it->second.size();
        }
        return 0;
    }

    size_t totalSubscriberCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t count = 0;
        for (const auto& entry : m_handlers) {
            count += entry.second.size();
        }
        return count;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_handlers.clear();
    }

private:
    TypedEventBus() = default;

    using KeyType = std::pair<std::type_index, std::string>;
    std::unordered_map<KeyType, std::vector<std::shared_ptr<std::function<void(const std::any&)>>>,
                       detail::TypeTopicHash, detail::TypeTopicEqual> m_handlers;
    mutable std::mutex m_mutex;
};

template<typename T>
class EventData {
public:
    EventData() = default;
    explicit EventData(T data) : m_data(std::move(data)) {}
    const T& data() const { return m_data; }
    T& data() { return m_data; }

private:
    T m_data;
};

}

#endif