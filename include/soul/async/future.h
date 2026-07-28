#ifndef SOUL_ASYNC_FUTURE_H
#define SOUL_ASYNC_FUTURE_H

// 注意: 本头文件为模板实现,以下 Qt 头文件无法前向声明:
//   - <QFuture>:        QFuture<T> 作为成员变量(值类型,需要完整定义)
//   - <QFutureWatcher>: 用于异步监听 future 完成并触发回调(避免线程池死锁)
// soul/logging/logger.h 已通过 detail 帮助函数隔离,
// 避免每个包含 future.h 的 TU 都传递依赖 logger.h。
#include <QFuture>
#include <QFutureWatcher>
#include <QThreadPool>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include "soul/async/future_detail.h"

namespace sc {

template<typename T>
class Future {
public:
    Future() = default;
    Future(QFuture<T> future) : m_future(std::move(future)) {}
    Future(const Future&) = default;
    Future(Future&&) = default;
    Future& operator=(const Future&) = default;
    Future& operator=(Future&&) = default;

    bool isFinished() const { return m_future.isFinished(); }
    bool isCanceled() const { return m_future.isCanceled(); }
    bool isRunning() const { return m_future.isRunning(); }

    void waitForFinished() {
        try {
            m_future.waitForFinished();
        } catch (const std::exception& e) {
            detail::logAsyncException("Future::waitForFinished", e.what());
        } catch (...) {
            // Blanket catch: wait-boundary barrier — must not propagate unknown exceptions to caller.
            detail::logAsyncUnknownException("Future::waitForFinished");
        }
        executeCallbacks();
    }

    T result() {
        auto value = m_future.result();
        executeCallbacks();
        return value;
    }

    template<typename F>
    auto then(F&& func) -> Future<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));

        if (m_future.isFinished()) {
            try {
                U result = func(m_future.result());
                QPromise<U> promise;
                promise.start();
                promise.addResult(result);
                promise.finish();
                return Future<U>(promise.future());
            } catch (...) {
                // Blanket catch: sync-then path must translate exceptions into a failed Future.
                QPromise<U> promise;
                promise.start();
                promise.setException(std::current_exception());
                promise.finish();
                return Future<U>(promise.future());
            }
        }

        auto promisePtr = std::make_shared<QPromise<U>>();
        promisePtr->start();
        auto future = promisePtr->future();

        // 使用专用线程池(不与 async() 共享 QThreadPool::globalInstance()),
        // 避免 then 任务阻塞等待 async 任务时线程池满死锁。
        QFuture<T> originalFuture = m_future;
        static auto s_thenPool = []() {
            auto pool = new QThreadPool();
            pool->setMaxThreadCount(QThread::idealThreadCount() * 2);
            return pool;
        }();
        s_thenPool->start([promisePtr, func = std::forward<F>(func), originalFuture]() mutable {
            try {
                T result = originalFuture.result();
                try {
                    promisePtr->addResult(func(std::move(result)));
                    promisePtr->finish();
                } catch (...) {
                    // Blanket catch: inner task exception captured into promise (must not escape).
                    promisePtr->setException(std::current_exception());
                    promisePtr->finish();
                }
            } catch (...) {
                // Blanket catch: outer future-result exception captured into promise.
                promisePtr->setException(std::current_exception());
                promisePtr->finish();
            }
        });

        return Future<U>(std::move(future));
    }

    template<typename F>
    void onSuccess(F&& func) {
        if (m_future.isFinished()) {
            try {
                func(m_future.result());
            } catch (const std::exception& e) {
                detail::logAsyncException("Future::onSuccess callback", e.what());
            } catch (...) {
                // Blanket catch: onSuccess callback must not propagate exceptions to caller.
                detail::logAsyncUnknownException("Future::onSuccess callback");
            }
            return;
        }
        ensureCallbacks();
        {
            std::lock_guard<std::mutex> lock(m_data->mutex);
            m_data->successCallbacks.push_back(std::forward<F>(func));
        }
        ensureWatcher();
    }

    template<typename F>
    void onFailure(F&& func) {
        if (m_future.isFinished()) {
            try {
                m_future.result();
            } catch (const std::exception& e) {
                func(e);
            } catch (...) {
                // Blanket catch: onFailure translates unknown exceptions to a generic runtime_error.
                func(std::runtime_error("Unknown exception"));
            }
            return;
        }
        ensureCallbacks();
        {
            std::lock_guard<std::mutex> lock(m_data->mutex);
            m_data->failureCallbacks.push_back(std::forward<F>(func));
        }
        ensureWatcher();
    }

    void cancel() { m_future.cancel(); }
    QFuture<T> qFuture() const { return m_future; }

private:
    struct CallbackData {
        std::vector<std::function<void(const T&)>> successCallbacks;
        std::vector<std::function<void(const std::exception&)>> failureCallbacks;
        // TSan-safe: onSuccess/onFailure may be called from any thread while
        // executeCallbacksStatic (triggered by QFutureWatcher::finished) iterates
        // and clears the vectors. All accesses must hold this mutex.
        mutable std::mutex mutex;
    };

    void ensureCallbacks() {
        if (!m_data) {
            m_data = std::make_shared<CallbackData>();
        }
    }

    // 使用 QFutureWatcher 异步监听 future 完成,自动触发 executeCallbacks。
    // 避免用户必须显式调用 waitForFinished()/result() 才能触发回调。
    // 注意: lambda 捕获 m_data(shared_ptr) 和 m_future(值拷贝),
    // 避免捕获裸 this 导致 Future 对象析构后 UAF。
    void ensureWatcher() {
        if (m_watcher) return;
        m_watcher = std::make_shared<QFutureWatcher<T>>();
        auto data = m_data;
        auto future = m_future;
        QObject::connect(m_watcher.get(), &QFutureWatcher<T>::finished, [data, future]() {
            executeCallbacksStatic(data, future);
        });
        m_watcher->setFuture(m_future);
    }

    // 静态版本:不依赖 this,供 ensureWatcher lambda 安全调用
    // TSan-safe: 持锁 swap 出回调列表,释放锁后执行用户回调(防死锁/防重入)。
    static void executeCallbacksStatic(std::shared_ptr<CallbackData> data, QFuture<T> future) {
        if (!data) return;
        std::vector<std::function<void(const T&)>> successCallbacks;
        std::vector<std::function<void(const std::exception&)>> failureCallbacks;
        {
            std::lock_guard<std::mutex> lock(data->mutex);
            successCallbacks.swap(data->successCallbacks);
            failureCallbacks.swap(data->failureCallbacks);
        }
        if (!successCallbacks.empty()) {
            try {
                T result = future.result();
                for (auto& cb : successCallbacks) {
                    cb(result);
                }
            } catch (const std::exception& e) {
                detail::logAsyncException("Future::executeCallbacks success", e.what());
            } catch (...) {
                // Blanket catch: success-callback dispatch must not propagate exceptions.
                detail::logAsyncUnknownException("Future::executeCallbacks success");
            }
        }
        if (!failureCallbacks.empty()) {
            try {
                future.result();
            } catch (const std::exception& e) {
                for (auto& cb : failureCallbacks) {
                    cb(e);
                }
            } catch (...) {
                // Blanket catch: failure-callback dispatch translates unknown exceptions to runtime_error.
                for (auto& cb : failureCallbacks) {
                    cb(std::runtime_error("Unknown exception"));
                }
            }
        }
    }

    void executeCallbacks() {
        executeCallbacksStatic(m_data, m_future);
    }

    QFuture<T> m_future;
    std::shared_ptr<CallbackData> m_data;
    std::shared_ptr<QFutureWatcher<T>> m_watcher;
};

template<typename F>
Future<std::invoke_result_t<F>> async(F&& func) {
    using ResultType = std::invoke_result_t<F>;

    auto promisePtr = std::make_shared<QPromise<ResultType>>();
    promisePtr->start();
    auto future = promisePtr->future();

    QThreadPool::globalInstance()->start([func = std::forward<F>(func), promisePtr]() mutable {
        try {
            promisePtr->addResult(func());
            promisePtr->finish();
        } catch (...) {
            // Blanket catch: async() task must translate exceptions into a failed Future.
            promisePtr->setException(std::current_exception());
            promisePtr->finish();
        }
    });

    return Future<ResultType>(std::move(future));
}

template<typename F>
Future<std::invoke_result_t<F>> asyncOnThreadPool(F&& func) {
    using ResultType = std::invoke_result_t<F>;

    auto promisePtr = std::make_shared<QPromise<ResultType>>();
    promisePtr->start();
    auto future = promisePtr->future();

    QThreadPool::globalInstance()->start([func = std::forward<F>(func), promisePtr]() mutable {
        try {
            promisePtr->addResult(func());
            promisePtr->finish();
        } catch (const std::exception&) {
            promisePtr->setException(std::current_exception());
            promisePtr->finish();
        } catch (...) {
            // Blanket catch: asyncOnThreadPool task translates unknown exceptions into failed Future.
            promisePtr->setException(std::current_exception());
            promisePtr->finish();
        }
    });

    return Future<ResultType>(std::move(future));
}

}

#endif
