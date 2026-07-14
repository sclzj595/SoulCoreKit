#ifndef SOUL_CORE_SINGLETON_H
#define SOUL_CORE_SINGLETON_H

#include <memory>

namespace sc {

/**
 * @class Singleton
 * @brief 线程安全的单例模板
 *
 * 使用 Meyer's Singleton 模式实现线程安全的单例。
 * C++11 及以上保证静态局部变量初始化的线程安全性。
 *
 * 使用方式：
 * @code
 * class MyClass : public Singleton<MyClass> {
 *     friend class Singleton<MyClass>;
 * private:
 *     MyClass() = default;
 * };
 *
 * MyClass::instance().doSomething();
 * @endcode
 *
 * @tparam T 单例类型
 */
template<typename T>
class Singleton {
public:
    /**
     * @brief 获取单例实例
     * @return 单例对象的引用
     */
    static T& instance() {
        static T inst;
        return inst;
    }

    /**
     * @brief 禁止拷贝构造
     */
    Singleton(const Singleton&) = delete;

    /**
     * @brief 禁止赋值操作
     */
    Singleton& operator=(const Singleton&) = delete;

protected:
    /**
     * @brief 受保护的构造函数
     */
    Singleton() = default;

    /**
     * @brief 受保护的析构函数
     */
    ~Singleton() = default;
};

/**
 * @class SharedSingleton
 * @brief 线程安全的共享指针单例模板
 *
 * 与 Singleton 类似，但返回 std::shared_ptr<T>，适用于需要共享所有权的场景。
 *
 * 使用方式：
 * @code
 * class MyClass : public SharedSingleton<MyClass> {
 *     friend class SharedSingleton<MyClass>;
 * private:
 *     MyClass() = default;
 * };
 *
 * auto instance = MyClass::instance();
 * @endcode
 *
 * @tparam T 单例类型
 * @see Singleton
 */
template<typename T>
class SharedSingleton {
public:
    /**
     * @brief 获取单例实例
     * @return 单例对象的 shared_ptr
     */
    static std::shared_ptr<T> instance() {
        static std::shared_ptr<T> inst = std::make_shared<T>();
        return inst;
    }

    /**
     * @brief 禁止拷贝构造
     */
    SharedSingleton(const SharedSingleton&) = delete;

    /**
     * @brief 禁止赋值操作
     */
    SharedSingleton& operator=(const SharedSingleton&) = delete;

protected:
    /**
     * @brief 受保护的构造函数
     */
    SharedSingleton() = default;

    /**
     * @brief 受保护的析构函数
     */
    ~SharedSingleton() = default;
};

}

#endif
