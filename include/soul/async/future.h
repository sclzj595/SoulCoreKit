#ifndef SOUL_ASYNC_FUTURE_H
#define SOUL_ASYNC_FUTURE_H

// 注意: 本头文件为模板实现,以下 Qt 头文件无法前向声明:
//   - <QFuture>:    QFuture<T> 作为成员变量(值类型,需要完整定义)
//   - <QThreadPool>: 调用 QThreadPool::globalInstance() 静态方法(需要完整定义)
// <QFutureWatcher> 已移除(未使用);soul/logging/logger.h 已通过 detail 帮助函数隔离,
// 避免每个包含 future.h 的 TU 都传递依赖 logger.h。
#include <QFuture>
#include <QThreadPool>
#include <functional>
#include <memory>
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

        QFuture<T> originalFuture = m_future;
        QThreadPool::globalInstance()->start([promisePtr, func = std::forward<F>(func), originalFuture]() mutable {
            try {
                T result = originalFuture.result();
                try {
                    promisePtr->addResult(func(std::move(result)));
                    promisePtr->finish();
                } catch (...) {
                    promisePtr->setException(std::current_exception());
                    promisePtr->finish();
                }
            } catch (...) {
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
                detail::logAsyncUnknownException("Future::onSuccess callback");
            }
            return;
        }
        ensureCallbacks();
        m_data->successCallbacks.push_back(std::forward<F>(func));
    }

    template<typename F>
    void onFailure(F&& func) {
        if (m_future.isFinished()) {
            try {
                m_future.result();
            } catch (const std::exception& e) {
                func(e);
            } catch (...) {
                func(std::runtime_error("Unknown exception"));
            }
            return;
        }
        ensureCallbacks();
        m_data->failureCallbacks.push_back(std::forward<F>(func));
    }

    void cancel() { m_future.cancel(); }
    QFuture<T> qFuture() const { return m_future; }

private:
    struct CallbackData {
        std::vector<std::function<void(const T&)>> successCallbacks;
        std::vector<std::function<void(const std::exception&)>> failureCallbacks;
    };

    void ensureCallbacks() {
        if (!m_data) {
            m_data = std::make_shared<CallbackData>();
        }
    }

    void executeCallbacks() {
        if (!m_data) return;
        if (!m_data->successCallbacks.empty()) {
            try {
                T result = m_future.result();
                for (auto& cb : m_data->successCallbacks) {
                    cb(result);
                }
            } catch (const std::exception& e) {
                detail::logAsyncException("Future::executeCallbacks success", e.what());
            } catch (...) {
                detail::logAsyncUnknownException("Future::executeCallbacks success");
            }
            m_data->successCallbacks.clear();
        }
        if (!m_data->failureCallbacks.empty()) {
            try {
                m_future.result();
            } catch (const std::exception& e) {
                for (auto& cb : m_data->failureCallbacks) {
                    cb(e);
                }
            } catch (...) {
                for (auto& cb : m_data->failureCallbacks) {
                    cb(std::runtime_error("Unknown exception"));
                }
            }
            m_data->failureCallbacks.clear();
        }
    }

    QFuture<T> m_future;
    std::shared_ptr<CallbackData> m_data;
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
            promisePtr->setException(std::current_exception());
            promisePtr->finish();
        }
    });

    return Future<ResultType>(std::move(future));
}

}

#endif
