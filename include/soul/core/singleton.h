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
    ~Singleton() = default;
};

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

    void shutdownAll() {
        std::vector<std::function<void()>> fns;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            fns.swap(m_shutdownFns);
        }
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
    ~SharedSingleton() = default;

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