#ifndef SOUL_ASYNC_FUTURE_H
#define SOUL_ASYNC_FUTURE_H

#include <QFuture>
#include <QFutureWatcher>
#include <QThreadPool>
#include <functional>
#include <memory>
#include "soul/core/result.h"

namespace sc {

/**
 * @class Future
 * @brief 异步结果封装类，基于 Qt 的 QFuture
 *
 * Future<T> 封装了异步操作的结果，支持链式操作和回调：
 * - then()：链式处理结果
 * - onSuccess()：成功时的回调
 * - onFailure()：失败时的回调
 * - waitForFinished()：阻塞等待完成
 *
 * 使用方式：
 * @code
 * Future<int> future = async([](){ return 42; });
 * future.then([](int result) {
 *     return result * 2;
 * }).onSuccess([](int result) {
 *     // result = 84
 * });
 * @endcode
 *
 * @tparam T 返回值类型
 * @see Promise, async, asyncOnThreadPool
 */
template<typename T>
class Future {
public:
    /**
     * @brief 默认构造函数
     */
    Future() = default;

    /**
     * @brief 构造函数（从 QFuture 创建）
     * @param future Qt 的 QFuture 对象
     */
    Future(QFuture<T> future) : m_future(std::move(future)) {}

    /**
     * @brief 拷贝构造函数
     */
    Future(const Future&) = default;

    /**
     * @brief 移动构造函数
     */
    Future(Future&&) = default;

    /**
     * @brief 拷贝赋值运算符
     */
    Future& operator=(const Future&) = default;

    /**
     * @brief 移动赋值运算符
     */
    Future& operator=(Future&&) = default;

    /**
     * @brief 检查任务是否已完成
     * @return 如果已完成返回 true
     */
    bool isFinished() const { return m_future.isFinished(); }

    /**
     * @brief 检查任务是否已取消
     * @return 如果已取消返回 true
     */
    bool isCanceled() const { return m_future.isCanceled(); }

    /**
     * @brief 检查任务是否正在运行
     * @return 如果正在运行返回 true
     */
    bool isRunning() const { return m_future.isRunning(); }

    /**
     * @brief 阻塞等待任务完成
     */
    void waitForFinished() { m_future.waitForFinished(); }

    /**
     * @brief 获取任务结果（阻塞调用）
     * @return 任务返回值
     * @note 如果任务抛出异常，此方法会重新抛出
     */
    T result() const { return m_future.result(); }

    /**
     * @brief 链式处理结果
     * @tparam F 处理函数类型
     * @param func 处理函数，接收前一个结果并返回新结果
     * @return 新的 Future 对象
     */
    template<typename F>
    auto then(F&& func) -> Future<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));

        QPromise<U> promise;
        auto future = promise.future();

        auto watcher = new QFutureWatcher<T>();
        QObject::connect(watcher, &QFutureWatcher<T>::finished, [watcher, promise = std::move(promise), func = std::forward<F>(func)]() mutable {
            try {
                T result = watcher->future().result();
                promise.start([result = std::move(result), func = std::move(func)]() mutable {
                    return func(std::move(result));
                });
            } catch (...) {
                promise.setException(std::current_exception());
            }
            watcher->deleteLater();
        });

        watcher->setFuture(m_future);
        return Future<U>(std::move(future));
    }

    /**
     * @brief 设置成功回调
     * @tparam F 回调函数类型
     * @param func 成功时的回调函数
     */
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

    /**
     * @brief 设置失败回调
     * @tparam F 回调函数类型
     * @param func 失败时的回调函数，接收异常对象
     */
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

    /**
     * @brief 取消任务
     */
    void cancel() { m_future.cancel(); }

    /**
     * @brief 获取底层的 QFuture 对象
     * @return QFuture 对象
     */
    QFuture<T> qFuture() const { return m_future; }

private:
    QFuture<T> m_future;
};

/**
 * @brief 在后台线程执行异步操作（使用 QPromise）
 * @tparam F 函数类型
 * @param func 要执行的函数
 * @return Future 对象
 */
template<typename F>
Future<std::invoke_result_t<F>> async(F&& func) {
    QPromise<std::invoke_result_t<F>> promise;
    auto future = promise.future();

    promise.start(std::forward<F>(func));
    return Future<std::invoke_result_t<F>>(std::move(future));
}

/**
 * @brief 在全局线程池执行异步操作
 * @tparam F 函数类型
 * @param func 要执行的函数
 * @return Future 对象
 * @see async
 */
template<typename F>
Future<std::invoke_result_t<F>> asyncOnThreadPool(F&& func) {
    QPromise<std::invoke_result_t<F>> promise;
    auto future = promise.future();

    QThreadPool::globalInstance()->start([promise = std::move(promise), func = std::forward<F>(func)]() mutable {
        try {
            promise.addResult(func());
            promise.finish();
        } catch (...) {
            promise.setException(std::current_exception());
        }
    });

    return Future<std::invoke_result_t<F>>(std::move(future));
}

}

#endif
