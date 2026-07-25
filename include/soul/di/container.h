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
#include "soul/core/result.h"

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
    std::function<void(void*)> deleter; // 类型擦除的删除器，避免 delete void*
};

class SC_DI_EXPORT Container {
public:
    using Creator = std::function<void*(const std::unordered_map<std::type_index, void*>&)>;
    using ScopeId = uint64_t;

    static Container& instance()
    {
        static Container inst;
        return inst;
    }

    ScopeId createScope() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        ScopeId id = m_nextScopeId++;
        m_scopes[id] = {};
        m_currentScopeId = id;
        return id;
    }

    void disposeScope(ScopeId scopeId) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_scopes.find(scopeId);
        if (it != m_scopes.end()) {
            for (auto& pair : it->second) {
                auto regIt = m_registrations.find(pair.first);
                if (regIt != m_registrations.end() && regIt->second.deleter) {
                    regIt->second.deleter(pair.second);
                }
            }
            m_scopes.erase(it);
            if (m_currentScopeId == scopeId) {
                m_currentScopeId = 0;
            }
        }
    }

    ScopeId currentScope() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_currentScopeId;
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
        info.deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
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
        info.deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
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
        info.deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
    }

    template<typename T>
    Result<std::shared_ptr<T>> resolve()
    {
        auto typeIdx = std::type_index(typeid(T));

        auto regIt = m_registrations.find(typeIdx);
        if (regIt == m_registrations.end()) {
            return Error(ErrorCode::NotFound, "Type not registered: " + std::string(typeid(T).name()));
        }

        if (regIt->second.initFlag &&
            regIt->second.initFlag->load(std::memory_order_acquire) &&
            regIt->second.singletonInstance) {
            return std::shared_ptr<T>(static_cast<T*>(regIt->second.singletonInstance), [](T*) {});
        }

        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_registrations.find(typeIdx);
        if (it == m_registrations.end()) {
            return Error(ErrorCode::NotFound, "Type not registered: " + std::string(typeid(T).name()));
        }

        auto& info = it->second;

        if (info.lifetime == Lifetime::Singleton) {
            if (info.initialized && info.singletonInstance) {
                return std::shared_ptr<T>(static_cast<T*>(info.singletonInstance), [](T*) {});
            }

            if (!info.creator) {
                return Error(ErrorCode::InvalidArgument, "Creator not set for singleton");
            }

            void* instance = info.creator(m_resolvedInstances);
            if (!instance) {
                return Error(ErrorCode::InternalError, "Creator returned null");
            }

            info.singletonInstance = instance;
            info.initialized = true;
            m_resolvedInstances[typeIdx] = instance;

            if (info.initFlag) {
                info.initFlag->store(true, std::memory_order_release);
            }

            return std::shared_ptr<T>(static_cast<T*>(instance), [](T*) {});
        }

        if (info.lifetime == Lifetime::Scoped) {
            if (m_currentScopeId != 0) {
                auto scopeIt = m_scopes.find(m_currentScopeId);
                if (scopeIt != m_scopes.end()) {
                    auto instIt = scopeIt->second.find(typeIdx);
                    if (instIt != scopeIt->second.end()) {
                        return std::shared_ptr<T>(static_cast<T*>(instIt->second), [](T*) {});
                    }
                }
            }

            if (!info.creator) {
                return Error(ErrorCode::InvalidArgument, "Creator not set for scoped");
            }

            void* instance = info.creator(m_resolvedInstances);
            if (!instance) {
                return Error(ErrorCode::InternalError, "Creator returned null");
            }

            if (m_currentScopeId != 0) {
                m_scopes[m_currentScopeId][typeIdx] = instance;
                return std::shared_ptr<T>(static_cast<T*>(instance), [](T*) {});
            }

            return std::shared_ptr<T>(static_cast<T*>(instance));
        }

        if (info.lifetime == Lifetime::Transient) {
            if (!info.creator) {
                return Error(ErrorCode::InvalidArgument, "Creator not set for transient");
            }
            void* instance = info.creator(m_resolvedInstances);
            if (!instance) {
                return Error(ErrorCode::InternalError, "Creator returned null");
            }
            return std::shared_ptr<T>(static_cast<T*>(instance));
        }

        return Error(ErrorCode::InternalError, "Unknown lifetime type");
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
                if (it->second.deleter) {
                    it->second.deleter(it->second.singletonInstance);
                }
            }
            m_registrations.erase(typeIdx);
            m_resolvedInstances.erase(typeIdx);
        }
    }

    void clear()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        for (auto& pair : m_scopes) {
            for (auto& instPair : pair.second) {
                auto regIt = m_registrations.find(instPair.first);
                if (regIt != m_registrations.end() && regIt->second.deleter) {
                    regIt->second.deleter(instPair.second);
                }
            }
        }
        m_scopes.clear();
        m_currentScopeId = 0;

        for (auto& pair : m_registrations) {
            if (pair.second.lifetime == Lifetime::Singleton &&
                pair.second.initialized &&
                pair.second.singletonInstance) {
                if (pair.second.deleter) {
                    pair.second.deleter(pair.second.singletonInstance);
                }
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
    std::unordered_map<ScopeId, std::unordered_map<std::type_index, void*>> m_scopes;
    ScopeId m_currentScopeId = 0;
    ScopeId m_nextScopeId = 1;
};

template<typename T>
struct SingletonWrapper {
    static Result<std::shared_ptr<T>> get()
    {
        return Container::instance().resolve<T>();
    }
};

} // namespace di
} // namespace sc

#endif
