#ifndef SOUL_CORE_SINGLETON_H
#define SOUL_CORE_SINGLETON_H

#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <algorithm>

namespace sc {

template<typename T>
class Singleton {
public:
    static T& instance() {
        static T inst;
        return inst;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

/// @brief 全局单例注册表 — 统一管理所有单例的销毁顺序
///
/// @par 设计目标
/// 解决 C++ 静态对象析构顺序不确定（Static Deinitialization Order Fiasco）问题。
/// 所有需要有序销毁的单例通过 registerShutdown() 注册回调，
/// shutdownAll() 以 LIFO 顺序执行（后注册先销毁，确保依赖倒置安全）。
///
/// @par 调用时机
/// @code
/// int main() {
///     Application app;
///     app.run();  // 内部初始化所有模块
///     // ...
///     SingletonRegistry::instance().shutdownAll();  // 1. 先清理单例
///     Container::instance().clear();                // 2. 再清理 DI 容器
///     return 0;
/// }
/// @endcode
///
/// @anchor shutdown_order
/// @par 关闭顺序（关键）
/// | 步骤 | 操作 | 原因 |
/// |------|------|------|
/// | 1 | shutdownAll() | 清理所有 Singleton/SharedSingleton，释放对 DI 对象的外部引用 |
/// | 2 | Container::clear() | 最后清理 DI 容器，此时所有外部引用已释放，shared_ptr 可安全析构 |
///
/// @warning 不得在 shutdownAll() 之前调用 Container::clear()，
///          否则 SharedSingleton 持有的 DI 对象可能成为悬空引用。
class SingletonRegistry {
public:
    static SingletonRegistry& instance() {
        static SingletonRegistry inst;
        return inst;
    }

    void registerShutdown(std::function<void()> shutdownFn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdownFns.push_back(std::move(shutdownFn));
    }

    /// @brief 逆序执行所有注册的 shutdown 回调（LIFO 确保依赖顺序）
    ///
    /// [v2.5.1] 调用时机: 应在进程退出前调用 shutdownAll()。
    /// 关闭顺序: shutdownAll() → Container::clear()（DI 容器最后清理）。
    /// 如果 SharedSingleton 注册了 DI 管理的对象，先清理 Singleton 再清理 DI 容器。
    void shutdownAll() {
        std::vector<std::function<void()>> fns;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            fns.swap(m_shutdownFns);
        }
        // LIFO: 后注册的先销毁，确保依赖倒置安全
        std::reverse(fns.begin(), fns.end());
        for (auto& fn : fns) {
            if (fn) fn();
        }
    }

private:
    SingletonRegistry() = default;
    ~SingletonRegistry() = default;

    std::mutex m_mutex;
    std::vector<std::function<void()>> m_shutdownFns;
};

template<typename T>
class SharedSingleton {
public:
    static std::shared_ptr<T> instance() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_instance) {
            m_instance = std::make_shared<T>();
            m_initialized = false;
        }
        return m_instance;
    }

    static void init() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_instance && !m_initialized) {
            m_instance->init();
            m_initialized = true;
            SingletonRegistry::instance().registerShutdown([]() {
                destroy();
            });
        }
    }

    static void destroy() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_instance) {
            m_instance->shutdown();
            m_instance.reset();
            m_initialized = false;
        }
    }

    static bool isInitialized() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_initialized;
    }

    SharedSingleton(const SharedSingleton&) = delete;
    SharedSingleton& operator=(const SharedSingleton&) = delete;

protected:
    SharedSingleton() = default;
    virtual ~SharedSingleton() = default;

private:
    static std::shared_ptr<T> m_instance;
    static std::mutex m_mutex;
    static bool m_initialized;
};

template<typename T>
std::shared_ptr<T> SharedSingleton<T>::m_instance = nullptr;

template<typename T>
std::mutex SharedSingleton<T>::m_mutex;

template<typename T>
bool SharedSingleton<T>::m_initialized = false;

}

#endif