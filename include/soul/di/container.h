#ifndef SOUL_DI_CONTAINER_H
#define SOUL_DI_CONTAINER_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

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
    // [v2.5.1] 使用 shared_ptr<void> 存储单例实例，共享控制块。
    // resolve() 返回 aliasing shared_ptr<T>，即使 clear() 重置了此成员，
    // 已分发的 shared_ptr 仍持有引用计数，实例不会被提前释放，消除 use-after-free。
    std::shared_ptr<void> singletonInstance;
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
            // [v2.5.1] shared_ptr<void> 自动管理生命周期，无需手动调用 deleter
            it->second.clear();
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
        info.singletonInstance.reset();
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
        // [v2.5.1] 包装为 shared_ptr<void>，no-op deleter（外部管理生命周期）
        info.singletonInstance = std::shared_ptr<T>(instance, [](T*) {});
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
        info.singletonInstance.reset();
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
        // [v2.5.1] 使用 aliasing shared_ptr 共享控制块，消除 use-after-free
        if (info.lifetime == Lifetime::Singleton) {
            if (info.initialized && info.singletonInstance) {
                return std::shared_ptr<T>(info.singletonInstance,
                    static_cast<T*>(info.singletonInstance.get()));
            }
            if (!info.creator) {
                return Error(ErrorCode::InvalidArgument, "Creator not set for named singleton");
            }
            void* instance = info.creator(emptyMap());
            if (!instance) return Error(ErrorCode::InternalError, "Creator returned null");
            info.singletonInstance = std::shared_ptr<void>(instance, info.deleter);
            info.initialized = true;
            return std::shared_ptr<T>(info.singletonInstance,
                static_cast<T*>(info.singletonInstance.get()));
        }
        if (info.lifetime == Lifetime::Scoped) {
            if (m_currentScopeId != 0) {
                auto scopeIt = m_scopes.find(m_currentScopeId);
                if (scopeIt != m_scopes.end()) {
                    auto instIt = scopeIt->second.find(typeIdx);
                    if (instIt != scopeIt->second.end()) {
                        return std::shared_ptr<T>(instIt->second,
                            static_cast<T*>(instIt->second.get()));
                    }
                }
            }
            if (!info.creator) {
                return Error(ErrorCode::InvalidArgument, "Creator not set for named scoped");
            }
            void* instance = info.creator(emptyMap());
            if (!instance) return Error(ErrorCode::InternalError, "Creator returned null");
            auto sp = std::shared_ptr<void>(instance, info.deleter);
            if (m_currentScopeId != 0) {
                m_scopes[m_currentScopeId][typeIdx] = sp;
                return std::shared_ptr<T>(sp, static_cast<T*>(sp.get()));
            }
            return std::shared_ptr<T>(sp, static_cast<T*>(sp.get()));
        }
        // Transient
        if (!info.creator) {
            return Error(ErrorCode::InvalidArgument, "Creator not set for named transient");
        }
        void* instance = info.creator(emptyMap());
        if (!instance) return Error(ErrorCode::InternalError, "Creator returned null");
        auto sp = std::shared_ptr<void>(instance, info.deleter);
        return std::shared_ptr<T>(sp, static_cast<T*>(sp.get()));
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
                return resolveNamed<T>(primIt->second);
            }
            return Result<std::shared_ptr<T>>(Error(ErrorCode::NotFound,
                QStringLiteral("Type not registered: %1").arg(QString::fromStdString(typeid(T).name()))));
        }

        auto& info = it->second;

        // [v2.5.1] 使用 aliasing shared_ptr 共享控制块
        // Singleton/Scoped: 返回的 shared_ptr 与内部存储的 shared_ptr<void> 共享控制块，
        // 即使 clear() 重置了内部存储，已分发的 shared_ptr 仍持有引用，实例不会被提前释放。

        if (info.lifetime == Lifetime::Singleton) {
            if (info.initialized && info.singletonInstance) {
                return std::shared_ptr<T>(info.singletonInstance,
                    static_cast<T*>(info.singletonInstance.get()));
            }

            if (!info.creator) {
                return Error(ErrorCode::InvalidArgument, "Creator not set for singleton");
            }

            void* instance = info.creator(emptyMap());
            if (!instance) {
                return Error(ErrorCode::InternalError, "Creator returned null");
            }

            info.singletonInstance = std::shared_ptr<void>(instance, info.deleter);
            info.initialized = true;

            if (info.initFlag) {
                info.initFlag->store(true, std::memory_order_release);
            }

            return std::shared_ptr<T>(info.singletonInstance,
                static_cast<T*>(info.singletonInstance.get()));
        }

        if (info.lifetime == Lifetime::Scoped) {
            if (m_currentScopeId != 0) {
                auto scopeIt = m_scopes.find(m_currentScopeId);
                if (scopeIt != m_scopes.end()) {
                    auto instIt = scopeIt->second.find(typeIdx);
                    if (instIt != scopeIt->second.end()) {
                        return std::shared_ptr<T>(instIt->second,
                            static_cast<T*>(instIt->second.get()));
                    }
                }
            }

            if (!info.creator) {
                return Error(ErrorCode::InvalidArgument, "Creator not set for scoped");
            }

            void* instance = info.creator(emptyMap());
            if (!instance) {
                return Error(ErrorCode::InternalError, "Creator returned null");
            }

            auto sp = std::shared_ptr<void>(instance, info.deleter);

            if (m_currentScopeId != 0) {
                m_scopes[m_currentScopeId][typeIdx] = sp;
                return std::shared_ptr<T>(sp, static_cast<T*>(sp.get()));
            }

            return std::shared_ptr<T>(sp, static_cast<T*>(sp.get()));
        }

        if (info.lifetime == Lifetime::Transient) {
            if (!info.creator) {
                return Error(ErrorCode::InvalidArgument, "Creator not set for transient");
            }
            void* instance = info.creator(emptyMap());
            if (!instance) {
                return Error(ErrorCode::InternalError, "Creator returned null");
            }
            auto sp = std::shared_ptr<void>(instance, info.deleter);
            return std::shared_ptr<T>(sp, static_cast<T*>(sp.get()));
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
            // [v2.5.1] shared_ptr 自动管理生命周期，重置即可
            it->second.singletonInstance.reset();
            it->second.initialized = false;
            m_registrations.erase(typeIdx);
        }
    }

    void clear()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        // [v2.5.1] shared_ptr<void> 自动管理生命周期。
        // 清除 scope 缓存和 registration 表时，只需重置 shared_ptr。
        // 已通过 resolve() 分发的 aliasing shared_ptr 仍持有控制块引用，
        // 实例不会被提前释放，彻底消除 use-after-free 隐患。
        m_scopes.clear();
        m_currentScopeId = 0;

        for (auto& pair : m_registrations) {
            pair.second.singletonInstance.reset();
            pair.second.initialized = false;
        }
        m_registrations.clear();

        for (auto& pair : m_namedRegistrations) {
            pair.second.singletonInstance.reset();
            pair.second.initialized = false;
        }
        m_namedRegistrations.clear();
        m_primaryImpls.clear();
    }

    size_t registrationCount() const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_registrations.size();
    }

    // [v1.9.4] DI 内省 — 返回已注册类型信息
    struct BeanInfo {
        std::string typeName;
        std::string lifetime;    // "transient" / "singleton" / "scoped"
        bool initialized = false;
    };

    // 获取所有已注册 Bean 的内省信息(对标 SpringBoot /actuator/beans)
    std::vector<BeanInfo> getRegisteredBeans() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        std::vector<BeanInfo> result;
        for (const auto& pair : m_registrations) {
            BeanInfo info;
            info.typeName = pair.first.name();
            switch (pair.second.lifetime) {
                case Lifetime::Transient: info.lifetime = "transient"; break;
                case Lifetime::Singleton: info.lifetime = "singleton"; break;
                case Lifetime::Scoped:    info.lifetime = "scoped";    break;
            }
            info.initialized = pair.second.initialized;
            result.push_back(info);
        }
        return result;
    }

private:
    Container() = default;
    ~Container() = default;

    Container(const Container&) = delete;
    Container& operator=(const Container&) = delete;

    // [v2.5.2] 所有 creator lambda 均忽略依赖映射参数，使用静态空 map 替代裸指针 m_resolvedInstances
    static const std::unordered_map<std::type_index, void*>& emptyMap() {
        static const std::unordered_map<std::type_index, void*> empty;
        return empty;
    }

    mutable std::recursive_mutex m_mutex;
    std::unordered_map<std::type_index, RegistrationInfo> m_registrations;
    // [v2.5.2] 已移除 m_resolvedInstances(void* map)。
    // singletonInstance(shared_ptr<void>) 已承担实例存储职责，m_resolvedInstances 是冗余的裸指针副本。
    // 所有 creator lambda 均忽略依赖映射参数，传递空 map 即可。
    // [v2.5.1] 使用 shared_ptr<void> 存储 scope 实例，共享控制块消除 use-after-free
    std::unordered_map<ScopeId, std::unordered_map<std::type_index, std::shared_ptr<void>>> m_scopes;
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
