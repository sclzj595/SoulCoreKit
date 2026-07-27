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
    bool owned = true;
    std::shared_ptr<std::atomic<bool>> initFlag;
    std::function<void(void*)> deleter;
};

// 复合键:type_index + qualifier name(用于 @Qualifier 支持)
struct NamedKey {
    std::type_index typeIdx;
    std::string name;
    bool operator==(const NamedKey& other) const {
        return typeIdx == other.typeIdx && name == other.name;
    }
};

struct NamedKeyHash {
    std::size_t operator()(const NamedKey& k) const {
        return k.typeIdx.hash_code() ^ std::hash<std::string>{}(k.name);
    }
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
                tryDeleteScopeInstance(pair.first, pair.second);
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
    [[nodiscard]] Result<void> bind(std::function<T*()> creator, Lifetime lifetime = Lifetime::Transient)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        if (m_registrations.find(typeIdx) != m_registrations.end()) {
            return Result<void>(Error(ErrorCode::AlreadyExists,
                "DI: type already registered"));
        }
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
        return Ok();
    }

    template<typename T>
    [[nodiscard]] Result<void> bindInstance(T* instance)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        if (m_registrations.find(typeIdx) != m_registrations.end()) {
            return Result<void>(Error(ErrorCode::AlreadyExists,
                "DI: type already registered"));
        }
        auto& info = m_registrations[typeIdx];
        info.lifetime = Lifetime::Singleton;
        info.creator = nullptr;
        info.interfaceType = typeIdx;
        info.singletonInstance = instance;
        info.initialized = true;
        info.owned = false;
        info.initFlag = std::make_shared<std::atomic<bool>>(true);
        info.deleter = [](void*) {};
        return Ok();
    }

    template<typename T>
    [[nodiscard]] Result<void> bindSingleton(std::function<T*()> creator)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        if (m_registrations.find(typeIdx) != m_registrations.end()) {
            return Result<void>(Error(ErrorCode::AlreadyExists,
                "DI: type already registered"));
        }
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
        return Ok();
    }

    // --- 便捷方法:对标 SpringBoot @Scope ---

    // 注册 Scoped 生命周期(每个 scope 一个实例)。
    // 需先调用 createScope() 创建作用域,resolve() 在作用域内返回同一实例。
    template<typename T>
    [[nodiscard]] Result<void> bindScoped(std::function<T*()> creator)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        if (m_registrations.find(typeIdx) != m_registrations.end()) {
            return Result<void>(Error(ErrorCode::AlreadyExists,
                "DI: type already registered"));
        }
        auto& info = m_registrations[typeIdx];
        info.lifetime = Lifetime::Scoped;
        info.creator = [creator](const std::unordered_map<std::type_index, void*>&) -> void* {
            return creator();
        };
        info.interfaceType = typeIdx;
        info.initFlag = std::make_shared<std::atomic<bool>>(false);
        info.deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
        return Ok();
    }

    // 注册 Transient 生命周期(每次 resolve() 创建新实例)。
    template<typename T>
    [[nodiscard]] Result<void> bindTransient(std::function<T*()> creator)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        if (m_registrations.find(typeIdx) != m_registrations.end()) {
            return Result<void>(Error(ErrorCode::AlreadyExists,
                "DI: type already registered"));
        }
        auto& info = m_registrations[typeIdx];
        info.lifetime = Lifetime::Transient;
        info.creator = [creator](const std::unordered_map<std::type_index, void*>&) -> void* {
            return creator();
        };
        info.interfaceType = typeIdx;
        info.initFlag = std::make_shared<std::atomic<bool>>(false);
        info.deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
        return Ok();
    }

    // --- Qualifier:按名称注册/解析(对标 @Qualifier) ---

    // 注册带名称限定符的实现。同一接口可注册多个命名实现。
    // name 不能为空,且 (type, name) 组合必须唯一。
    template<typename T>
    [[nodiscard]] Result<void> bindNamed(const std::string& name, std::function<T*()> creator,
                           Lifetime lifetime = Lifetime::Transient)
    {
        if (name.empty()) {
            return Result<void>(Error(ErrorCode::InvalidArgument,
                "DI: qualifier name cannot be empty"));
        }
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        // 复合键: type_index + name
        auto typeIdx = std::type_index(typeid(T));
        NamedKey key{typeIdx, name};
        if (m_namedRegistrations.find(key) != m_namedRegistrations.end()) {
            return Result<void>(Error(ErrorCode::AlreadyExists,
                "DI: named type already registered"));
        }
        auto& info = m_namedRegistrations[key];
        info.lifetime = lifetime;
        info.creator = [creator](const std::unordered_map<std::type_index, void*>&) -> void* {
            return creator();
        };
        info.interfaceType = typeIdx;
        info.initFlag = std::make_shared<std::atomic<bool>>(false);
        info.deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
        return Ok();
    }

    // 按名称解析(对标 @Qualifier)。
    template<typename T>
    Result<std::shared_ptr<T>> resolveNamed(const std::string& name)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        NamedKey key{typeIdx, name};
        auto it = m_namedRegistrations.find(key);
        if (it == m_namedRegistrations.end()) {
            return Result<std::shared_ptr<T>>(Error(ErrorCode::NotFound,
                QStringLiteral("Named type not registered: %1").arg(QString::fromStdString(name))));
        }
        auto& info = it->second;
        // Named 注册支持 Transient/Singleton/Scoped 三种生命周期
        if (info.lifetime == Lifetime::Singleton) {
            if (info.initialized && info.singletonInstance) {
                return std::shared_ptr<T>(static_cast<T*>(info.singletonInstance), [](T*) {});
            }
            if (!info.creator) {
                return Error(ErrorCode::InvalidArgument, "Creator not set for named singleton");
            }
            void* instance = info.creator(m_resolvedInstances);
            if (!instance) return Error(ErrorCode::InternalError, "Creator returned null");
            info.singletonInstance = instance;
            info.initialized = true;
            return std::shared_ptr<T>(static_cast<T*>(instance), [](T*) {});
        }
        if (info.lifetime == Lifetime::Scoped) {
            // 与 resolve() 的 Scoped 逻辑对称:查 scope 缓存 → 缓存实例 → no-op deleter
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
                return Error(ErrorCode::InvalidArgument, "Creator not set for named scoped");
            }
            void* instance = info.creator(m_resolvedInstances);
            if (!instance) return Error(ErrorCode::InternalError, "Creator returned null");
            if (m_currentScopeId != 0) {
                m_scopes[m_currentScopeId][typeIdx] = instance;
                return std::shared_ptr<T>(static_cast<T*>(instance), [](T*) {});
            }
            // 无活跃 scope 时退化为 Transient(不缓存,由 shared_ptr 默认删除器管理)
            return std::shared_ptr<T>(static_cast<T*>(instance));
        }
        // Transient
        if (!info.creator) {
            return Error(ErrorCode::InvalidArgument, "Creator not set for named transient");
        }
        void* instance = info.creator(m_resolvedInstances);
        if (!instance) return Error(ErrorCode::InternalError, "Creator returned null");
        return std::shared_ptr<T>(static_cast<T*>(instance));
    }

    // --- Primary:默认实现(对标 @Primary) ---
    // 当同一接口注册了多个实现(通过 bindNamed),可标记其中一个为 Primary,
    // resolve<T>() 在未指定名称时优先返回 Primary 实现。
    template<typename T>
    Result<void> setPrimary(const std::string& name)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        NamedKey key{typeIdx, name};
        if (m_namedRegistrations.find(key) == m_namedRegistrations.end()) {
            return Result<void>(Error(ErrorCode::NotFound,
                "DI: cannot set primary, named registration not found"));
        }
        m_primaryImpls[typeIdx] = name;
        return Ok();
    }

    // 检查某个命名实现是否为 Primary
    template<typename T>
    bool isPrimary(const std::string& name) const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto typeIdx = std::type_index(typeid(T));
        auto it = m_primaryImpls.find(typeIdx);
        return it != m_primaryImpls.end() && it->second == name;
    }

    template<typename T>
    Result<std::shared_ptr<T>> resolve()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        auto typeIdx = std::type_index(typeid(T));

        auto it = m_registrations.find(typeIdx);
        if (it == m_registrations.end()) {
            // 回退:如果有 Primary 命名实现,返回 Primary
            auto primIt = m_primaryImpls.find(typeIdx);
            if (primIt != m_primaryImpls.end()) {
                // 释放锁后调用 resolveNamed(避免递归锁开销,但 recursive_mutex 可重入)
                return resolveNamed<T>(primIt->second);
            }
            return Result<std::shared_ptr<T>>(Error(ErrorCode::NotFound,
                QStringLiteral("Type not registered: %1").arg(QString::fromStdString(typeid(T).name()))));
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
                it->second.singletonInstance &&
                it->second.owned) {
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
                tryDeleteScopeInstance(instPair.first, instPair.second);
            }
        }
        m_scopes.clear();
        m_currentScopeId = 0;

        for (auto& pair : m_registrations) {
            if (pair.second.lifetime == Lifetime::Singleton &&
                pair.second.initialized &&
                pair.second.singletonInstance &&
                pair.second.owned) {
                if (pair.second.deleter) {
                    pair.second.deleter(pair.second.singletonInstance);
                }
            }
        }
        m_registrations.clear();
        m_resolvedInstances.clear();

        // 清理 named 注册的单例
        for (auto& pair : m_namedRegistrations) {
            if (pair.second.lifetime == Lifetime::Singleton &&
                pair.second.initialized &&
                pair.second.singletonInstance &&
                pair.second.owned) {
                if (pair.second.deleter) {
                    pair.second.deleter(pair.second.singletonInstance);
                }
            }
        }
        m_namedRegistrations.clear();
        m_primaryImpls.clear();
    }

    size_t registrationCount() const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_registrations.size();
    }

private:
    // 查找 scope 实例的 deleter:先查 m_registrations,再回退到 m_namedRegistrations。
    // 原因: named Scoped 实例通过 resolveNamed 写入 m_scopes,但其 deleter 存储在
    // m_namedRegistrations 而非 m_registrations 中。
    bool tryDeleteScopeInstance(std::type_index typeIdx, void* instance) {
        auto regIt = m_registrations.find(typeIdx);
        if (regIt != m_registrations.end() && regIt->second.deleter && regIt->second.owned) {
            regIt->second.deleter(instance);
            return true;
        }
        // 回退到 named 注册表:遍历所有 NamedKey,匹配 typeIdx
        for (auto& np : m_namedRegistrations) {
            if (np.first.typeIdx == typeIdx && np.second.deleter && np.second.owned) {
                np.second.deleter(instance);
                return true;
            }
        }
        return false;
    }

    Container() = default;
    ~Container() = default;

    Container(const Container&) = delete;
    Container& operator=(const Container&) = delete;

    mutable std::recursive_mutex m_mutex;
    std::unordered_map<std::type_index, RegistrationInfo> m_registrations;
    std::unordered_map<std::type_index, void*> m_resolvedInstances;
    std::unordered_map<ScopeId, std::unordered_map<std::type_index, void*>> m_scopes;
    std::unordered_map<NamedKey, RegistrationInfo, NamedKeyHash> m_namedRegistrations;
    std::unordered_map<std::type_index, std::string> m_primaryImpls;
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
