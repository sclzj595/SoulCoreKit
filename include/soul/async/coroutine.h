#ifndef SOUL_ASYNC_COROUTINE_H
#define SOUL_ASYNC_COROUTINE_H

// Only available when C++20 is enabled
#ifdef SOUL_ENABLE_CXX20

#include <coroutine>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <type_traits>
#include <utility>

namespace sc {

// ============================================================================
// Task<T> — C++20 coroutine return type
// ============================================================================
// Simple coroutine Task type that supports co_await.
// Usage:
//   Task<int> computeAsync() {
//       co_return 42;
//   }
//
//   Task<int> caller() {
//       int result = co_await computeAsync();
//       co_return result * 2;
//   }

template<typename T = void>
class Task {
public:
    struct promise_type {
        T m_value;
        std::exception_ptr m_exception;
        std::coroutine_handle<> m_continuation;
        bool m_ready = false;
        mutable std::mutex m_mutex;              ///< [v1.9.2] 保护 get() 等待 + TSan 安全
        std::condition_variable m_cv;            ///< [v1.9.2] 替代自旋等待

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }

        auto final_suspend() noexcept {
            struct FinalAwaiter {
                promise_type* m_promise;

                bool await_ready() noexcept { return false; }

                void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    // TSan-safe: 持锁读取 m_continuation,与 await_suspend() 的写同步
                    std::coroutine_handle<> continuation;
                    {
                        std::lock_guard<std::mutex> lock(m_promise->m_mutex);
                        continuation = m_promise->m_continuation;
                    }
                    if (continuation) {
                        continuation.resume();
                    }
                }

                void await_resume() noexcept {}
            };
            return FinalAwaiter{this};
        }

        void return_value(T value) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_value = std::move(value);
                m_ready = true;
            }
            m_cv.notify_all();
        }

        void unhandled_exception() {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_exception = std::current_exception();
                m_ready = true;
            }
            m_cv.notify_all();
        }
    };

    Task(std::coroutine_handle<promise_type> h) : m_handle(h) {}
    Task(Task&& other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (m_handle) m_handle.destroy();
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    ~Task() {
        if (m_handle) m_handle.destroy();
    }

    // Awaiter for co_await
    // TSan-safe: 持锁读取 m_ready,与 return_value()/unhandled_exception() 的写同步
    // 注: 非 noexcept,因为 std::lock_guard 构造可能抛出 std::system_error
    bool await_ready() const {
        std::lock_guard<std::mutex> lock(m_handle.promise().m_mutex);
        return m_handle.promise().m_ready;
    }

    void await_suspend(std::coroutine_handle<> continuation) {
        // TSan-safe: 持锁写入 m_continuation,与 final_suspend() 的读同步
        std::lock_guard<std::mutex> lock(m_handle.promise().m_mutex);
        m_handle.promise().m_continuation = continuation;
    }

    T await_resume() {
        // TSan-safe: 持锁读取 m_exception 和 m_value,与 return_value()/unhandled_exception() 的写同步
        std::lock_guard<std::mutex> lock(m_handle.promise().m_mutex);
        auto& promise = m_handle.promise();
        if (promise.m_exception) {
            std::rethrow_exception(promise.m_exception);
        }
        return std::move(promise.m_value);
    }

    // [v1.9.2] 使用条件变量替代自旋等待
    T get() {
        std::unique_lock<std::mutex> lock(m_handle.promise().m_mutex);
        m_handle.promise().m_cv.wait(lock, [this] {
            return m_handle.promise().m_ready;
        });
        auto& promise = m_handle.promise();
        if (promise.m_exception) {
            std::rethrow_exception(promise.m_exception);
        }
        return std::move(promise.m_value);
    }

private:
    std::coroutine_handle<promise_type> m_handle;
};

// Task<void> specialization
template<>
class Task<void> {
public:
    struct promise_type {
        std::exception_ptr m_exception;
        std::coroutine_handle<> m_continuation;
        bool m_ready = false;
        mutable std::mutex m_mutex;              ///< [v1.9.2] 保护 get() 等待 + TSan 安全
        std::condition_variable m_cv;            ///< [v1.9.2] 替代自旋等待

        Task<void> get_return_object() {
            return Task<void>{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }

        auto final_suspend() noexcept {
            struct FinalAwaiter {
                promise_type* m_promise;
                bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    // TSan-safe: 持锁读取 m_continuation,与 await_suspend() 的写同步
                    std::coroutine_handle<> continuation;
                    {
                        std::lock_guard<std::mutex> lock(m_promise->m_mutex);
                        continuation = m_promise->m_continuation;
                    }
                    if (continuation) {
                        continuation.resume();
                    }
                }
                void await_resume() noexcept {}
            };
            return FinalAwaiter{this};
        }

        void return_void() {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_ready = true;
            }
            m_cv.notify_all();
        }

        void unhandled_exception() {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_exception = std::current_exception();
                m_ready = true;
            }
            m_cv.notify_all();
        }
    };

    Task(std::coroutine_handle<promise_type> h) : m_handle(h) {}
    Task(Task&& other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (m_handle) m_handle.destroy();
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    ~Task() {
        if (m_handle) m_handle.destroy();
    }

    // TSan-safe: 持锁读取 m_ready,与 return_void()/unhandled_exception() 的写同步
    // 注: 非 noexcept,因为 std::lock_guard 构造可能抛出 std::system_error
    bool await_ready() const {
        std::lock_guard<std::mutex> lock(m_handle.promise().m_mutex);
        return m_handle.promise().m_ready;
    }

    void await_suspend(std::coroutine_handle<> continuation) {
        // TSan-safe: 持锁写入 m_continuation,与 final_suspend() 的读同步
        std::lock_guard<std::mutex> lock(m_handle.promise().m_mutex);
        m_handle.promise().m_continuation = continuation;
    }

    void await_resume() {
        // TSan-safe: 持锁读取 m_exception,与 unhandled_exception() 的写同步
        std::lock_guard<std::mutex> lock(m_handle.promise().m_mutex);
        auto& promise = m_handle.promise();
        if (promise.m_exception) {
            std::rethrow_exception(promise.m_exception);
        }
    }

    // [v1.9.2] 使用条件变量替代自旋等待
    void get() {
        std::unique_lock<std::mutex> lock(m_handle.promise().m_mutex);
        m_handle.promise().m_cv.wait(lock, [this] {
            return m_handle.promise().m_ready;
        });
        auto& promise = m_handle.promise();
        if (promise.m_exception) {
            std::rethrow_exception(promise.m_exception);
        }
    }

private:
    std::coroutine_handle<promise_type> m_handle;
};

// ============================================================================
// Generator<T> — Simple generator coroutine
// ============================================================================
// Usage:
//   Generator<int> range(int n) {
//       for (int i = 0; i < n; ++i) {
//           co_yield i;
//       }
//   }

template<typename T>
class Generator {
public:
    struct promise_type {
        T m_current;
        std::exception_ptr m_exception;

        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(T value) {
            m_current = std::move(value);
            return {};
        }

        void return_void() {}
        void unhandled_exception() {
            m_exception = std::current_exception();
        }
    };

    struct Iterator {
        std::coroutine_handle<promise_type> m_handle;

        Iterator& operator++() {
            m_handle.resume();
            if (m_handle.done()) {
                m_handle = nullptr;
            }
            return *this;
        }

        T operator*() const { return m_handle.promise().m_current; }
        bool operator!=(const Iterator& other) const { return m_handle != other.m_handle; }
    };

    Generator(std::coroutine_handle<promise_type> h) : m_handle(h) {}
    Generator(Generator&& other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (m_handle) m_handle.destroy();
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

    ~Generator() {
        if (m_handle) m_handle.destroy();
    }

    Iterator begin() {
        m_handle.resume();
        if (m_handle.done()) return {nullptr};
        return {m_handle};
    }

    Iterator end() { return {nullptr}; }

private:
    std::coroutine_handle<promise_type> m_handle;
};

} // namespace sc

#endif // SOUL_ENABLE_CXX20
#endif // SOUL_ASYNC_COROUTINE_H