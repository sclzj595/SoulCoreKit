#ifndef SOUL_ASYNC_FUTURE_H
#define SOUL_ASYNC_FUTURE_H

#include <QFuture>
#include <QFutureWatcher>
#include <QThreadPool>
#include <functional>
#include <memory>
#include "soul/core/result.h"

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

    void waitForFinished() { m_future.waitForFinished(); }
    T result() const { return m_future.result(); }

    template<typename F>
    auto then(F&& func) -> Future<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));

        QPromise<U> promise;
        auto future = promise.future();

        auto watcher = new QFutureWatcher<T>();
        QObject::connect(watcher, &QFutureWatcher<T>::finished, [watcher, promisePtr = std::make_shared<QPromise<U>>(std::move(promise)), func = std::forward<F>(func)]() mutable {
            try {
                T result = watcher->future().result();
                QThreadPool::globalInstance()->start([result = std::move(result), func = std::move(func), promisePtr]() mutable {
                    try {
                        promisePtr->start();
                        promisePtr->addResult(func(std::move(result)));
                        promisePtr->finish();
                    } catch (...) {
                        promisePtr->setException(std::current_exception());
                    }
                });
            } catch (...) {
                promisePtr->start();
                promisePtr->setException(std::current_exception());
            }
            watcher->deleteLater();
        });

        watcher->setFuture(m_future);
        return Future<U>(std::move(future));
    }

    template<typename F>
    void onSuccess(F&& func) {
        auto watcher = new QFutureWatcher<T>();
        QObject::connect(watcher, &QFutureWatcher<T>::finished, [watcher, func = std::forward<F>(func)]() {
            try {
                func(watcher->future().result());
            } catch (...) {
            }
            watcher->deleteLater();
        });
        watcher->setFuture(m_future);
    }

    template<typename F>
    void onFailure(F&& func) {
        auto watcher = new QFutureWatcher<T>();
        QObject::connect(watcher, &QFutureWatcher<T>::finished, [watcher, func = std::forward<F>(func)]() {
            try {
                watcher->future().result();
            } catch (const std::exception& e) {
                func(e);
            } catch (...) {
                func(std::runtime_error("Unknown exception"));
            }
            watcher->deleteLater();
        });
        watcher->setFuture(m_future);
    }

    void cancel() { m_future.cancel(); }
    QFuture<T> qFuture() const { return m_future; }

private:
    QFuture<T> m_future;
};

template<typename F>
Future<std::invoke_result_t<F>> async(F&& func) {
    using ResultType = std::invoke_result_t<F>;

    QPromise<ResultType> promise;
    auto future = promise.future();

    auto promisePtr = std::make_shared<QPromise<ResultType>>(std::move(promise));
    QThreadPool::globalInstance()->start([func = std::forward<F>(func), promisePtr]() mutable {
        try {
            promisePtr->start();
            promisePtr->addResult(func());
            promisePtr->finish();
        } catch (...) {
            promisePtr->setException(std::current_exception());
        }
    });

    return Future<ResultType>(std::move(future));
}

template<typename F>
Future<std::invoke_result_t<F>> asyncOnThreadPool(F&& func) {
    using ResultType = std::invoke_result_t<F>;

    QPromise<ResultType> promise;
    auto future = promise.future();

    auto promisePtr = std::make_shared<QPromise<ResultType>>(std::move(promise));
    QThreadPool::globalInstance()->start([func = std::forward<F>(func), promisePtr]() mutable {
        try {
            promisePtr->start();
            promisePtr->addResult(func());
            promisePtr->finish();
        } catch (...) {
            promisePtr->setException(std::current_exception());
        }
    });

    return Future<ResultType>(std::move(future));
}

}

#endif