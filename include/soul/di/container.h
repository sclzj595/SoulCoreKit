#ifndef SOUL_DI_CONTAINER_H
#define SOUL_DI_CONTAINER_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "di_global.h"

namespace sc {
namespace di {

enum class Lifetime {
    Transient,
    Singleton,
    Scoped
};

struct SC_DI_EXPORT RegistrationInfo {
    Lifetime lifetime = Lifetime::Transient;
    std::function<void*(const std::unordered_map<std::type_index, void*>&)> creator;
    std::type_index interfaceType = std::type_index(typeid(void));
    void* singletonInstance = nullptr;
    bool initialized = false;
    std::shared_ptr<std::atomic<bool>> initFlag;
};

class SC_DI_EXPORT Container {
public:
    using Creator = std::function<void*(const std::unordered_map<std::type_index, void*>&)>;

    static Container& instance()
    {
        static Container inst;
        return inst;
    }

    template<typename T>
    void bind(std::function<T*()> creator, Lifetime lifetime = Lifetime::Transient)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        auto& info = m_registrations[typeIdx];
        info.lifetime = lifetime;
        info.creator = [creator](const std::unordered_map<std::type_index, void*>&) -> void* {
            return creator();
        };
        info.interfaceType = typeIdx;
        info.singletonInstance = nullptr;
        info.initialized = false;
        info.initFlag = std::make_shared<std::atomic<bool>>(false);
    }

    template<typename T>
    void bindInstance(T* instance)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        auto& info = m_registrations[typeIdx];
        info.lifetime = Lifetime::Singleton;
        info.creator = nullptr;
        info.interfaceType = typeIdx;
        info.singletonInstance = instance;
        info.initialized = true;
        info.initFlag = std::make_shared<std::atomic<bool>>(true);
    }

    template<typename T>
    void bindSingleton(std::function<T*()> creator)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        auto& info = m_registrations[typeIdx];
        info.lifetime = Lifetime::Singleton;
        info.creator = [creator](const std::unordered_map<std::type_index, void*>&) -> void* {
            return creator();
        };
        info.interfaceType = typeIdx;
        info.singletonInstance = nullptr;
        info.initialized = false;
        info.initFlag = std::make_shared<std::atomic<bool>>(false);
    }

    template<typename T>
    std::shared_ptr<T> resolve()
    {
        auto typeIdx = std::type_index(typeid(T));

        auto regIt = m_registrations.find(typeIdx);
        if (regIt == m_registrations.end()) {
            return nullptr;
        }

        if (regIt->second.initFlag &&
            regIt->second.initFlag->load(std::memory_order_acquire) &&
            regIt->second.singletonInstance) {
            return std::shared_ptr<T>(static_cast<T*>(regIt->second.singletonInstance), [](T*) {});
        }

        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_registrations.find(typeIdx);
        if (it == m_registrations.end()) {
            return nullptr;
        }

        auto& info = it->second;

        if (info.lifetime == Lifetime::Singleton) {
            if (info.initialized && info.singletonInstance) {
                return std::shared_ptr<T>(static_cast<T*>(info.singletonInstance), [](T*) {});
            }

            if (!info.creator) {
                return nullptr;
            }

            void* instance = info.creator(m_resolvedInstances);
            if (!instance) {
                return nullptr;
            }

            info.singletonInstance = instance;
            info.initialized = true;
            m_resolvedInstances[typeIdx] = instance;

            if (info.initFlag) {
                info.initFlag->store(true, std::memory_order_release);
            }

            return std::shared_ptr<T>(static_cast<T*>(instance), [](T*) {});
        }

        if (info.lifetime == Lifetime::Transient) {
            if (!info.creator) {
                return nullptr;
            }
            void* instance = info.creator(m_resolvedInstances);
            if (!instance) {
                return nullptr;
            }
            return std::shared_ptr<T>(static_cast<T*>(instance));
        }

        return nullptr;
    }

    template<typename T>
    bool isRegistered() const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_registrations.find(std::type_index(typeid(T))) != m_registrations.end();
    }

    template<typename T>
    void unregister()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        auto it = m_registrations.find(typeIdx);
        if (it != m_registrations.end()) {
            if (it->second.lifetime == Lifetime::Singleton &&
                it->second.initialized &&
                it->second.singletonInstance) {
                delete static_cast<T*>(it->second.singletonInstance);
            }
            m_registrations.erase(typeIdx);
            m_resolvedInstances.erase(typeIdx);
        }
    }

    void clear()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        for (auto& pair : m_registrations) {
            if (pair.second.lifetime == Lifetime::Singleton &&
                pair.second.initialized &&
                pair.second.singletonInstance) {
                delete pair.second.singletonInstance;
            }
        }
        m_registrations.clear();
        m_resolvedInstances.clear();
    }

    size_t registrationCount() const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_registrations.size();
    }

private:
    Container() = default;
    ~Container() = default;

    Container(const Container&) = delete;
    Container& operator=(const Container&) = delete;

    mutable std::recursive_mutex m_mutex;
    std::unordered_map<std::type_index, RegistrationInfo> m_registrations;
    std::unordered_map<std::type_index, void*> m_resolvedInstances;
};

template<typename T>
struct SingletonWrapper {
    static std::shared_ptr<T> get()
    {
        return Container::instance().resolve<T>();
    }
};

} // namespace di
} // namespace sc

#endif
