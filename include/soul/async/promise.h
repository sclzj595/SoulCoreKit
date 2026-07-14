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
        m_promise.start(std::move(func));
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
        m_promise.start(std::move(func));
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
