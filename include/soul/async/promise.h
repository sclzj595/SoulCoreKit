#ifndef SOUL_ASYNC_PROMISE_H
#define SOUL_ASYNC_PROMISE_H

#include <QPromise>
#include <QFuture>
#include <functional>
#include "soul/async/future.h"

namespace sc {

template<typename T>
class Promise {
public:
    Promise() = default;

    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    Promise(Promise&&) = default;
    Promise& operator=(Promise&&) = default;

    Future<T> future() {
        return Future<T>(m_promise.future());
    }

    void start(std::function<T()> func) {
        // Qt 6 QPromise::start 期望 std::function<void(QPromise<T>&)>,
        // 这里适配用户传入的无参函数,并在内部完成 addResult+finish。
        m_promise.start([func = std::move(func)](QPromise<T>& promise) {
            try {
                if (promise.isCanceled()) return;
                T result = func();
                promise.addResult(std::move(result));
                promise.finish();
            } catch (...) {
                // Blanket catch: Promise<T>::start must translate task exceptions into a failed promise.
                promise.setException(std::current_exception());
            }
        });
    }

    void addResult(const T& value) {
        m_promise.addResult(value);
    }

    void addResult(T&& value) {
        m_promise.addResult(std::move(value));
    }

    void finish() {
        m_promise.finish();
    }

    void setException(std::exception_ptr exception) {
        m_promise.setException(std::move(exception));
    }

    bool isCanceled() const {
        return m_promise.isCanceled();
    }

private:
    QPromise<T> m_promise;
};

template<>
class Promise<void> {
public:
    Promise() = default;

    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    Promise(Promise&&) = default;
    Promise& operator=(Promise&&) = default;

    Future<void> future() {
        return Future<void>(m_promise.future());
    }

    void start(std::function<void()> func) {
        // Qt 6 QPromise::start 期望 std::function<void(QPromise<void>&)>,
        // 这里适配用户传入的无参函数。
        m_promise.start([func = std::move(func)](QPromise<void>& promise) {
            try {
                if (promise.isCanceled()) return;
                func();
                promise.finish();
            } catch (...) {
                // Blanket catch: Promise<void>::start must translate task exceptions into a failed promise.
                promise.setException(std::current_exception());
            }
        });
    }

    void finish() {
        m_promise.finish();
    }

    void setException(std::exception_ptr exception) {
        m_promise.setException(std::move(exception));
    }

    bool isCanceled() const {
        return m_promise.isCanceled();
    }

private:
    QPromise<void> m_promise;
};

}

#endif
