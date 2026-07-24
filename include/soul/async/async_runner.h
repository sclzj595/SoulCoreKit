#ifndef SOUL_ASYNC_ASYNC_RUNNER_H
#define SOUL_ASYNC_ASYNC_RUNNER_H

#include <functional>
#include <QThread>

namespace sc {

/**
 * @class AsyncRunner
 * @brief 异步执行器，提供多种异步执行方式
 *
 * AsyncRunner 是静态工具类，提供：
 * - 异步执行任务
 * - 异步执行任务并返回结果到主线程
 * - 在主线程执行任务
 * - 延迟执行任务
 *
 * 使用方式：
 * @code
 * // 异步执行
 * AsyncRunner::runAsync([](){
 *     // 后台任务
 * });
 *
 * // 异步执行并返回结果
 * AsyncRunner::runAsyncResult([](){
 *     return fetchData();
 * }, [](const Data& data){
 *     // 在主线程处理结果
 * });
 *
 * // 在主线程执行
 * AsyncRunner::runOnMainThread([](){
 *     // 更新 UI
 * });
 *
 * // 延迟执行
 * AsyncRunner::runDelayed([](){
 *     // 延迟任务
 * }, 1000);
 * @endcode
 *
 * @see Task
 */
class AsyncRunner {
public:
    /**
     * @brief 任务函数类型
     */
    using Task = std::function<void()>;

    /**
     * @brief 结果回调类型
     */
    using ResultCallback = std::function<void(bool)>;

    /**
     * @brief 异步执行任务
     * @param task 任务函数
     */
    static void runAsync(const Task& task);

    /**
     * @brief 异步执行任务（带回调）
     * @param task 任务函数
     * @param callback 完成回调
     */
    static void runAsync(const Task& task, const ResultCallback& callback);

    /**
     * @brief 异步执行任务并返回结果到主线程
     * @tparam R 返回值类型
     * @param task 返回值的任务函数
     * @param callback 结果回调（在主线程执行）
     */
    template<typename R>
    static void runAsyncResult(const std::function<R()>& task, const std::function<void(R)>& callback);

    /**
     * @brief 在主线程执行任务
     * @param task 任务函数
     */
    static void runOnMainThread(const Task& task);

    /**
     * @brief 延迟执行任务
     * @param task 任务函数
     * @param milliseconds 延迟时间（毫秒）
     */
    static void runDelayed(const Task& task, int milliseconds);
};

template<typename R>
void AsyncRunner::runAsyncResult(const std::function<R()>& task, const std::function<void(R)>& callback) {
    QThread::create([task, callback]() {
        R result = task();
        runOnMainThread([callback, result]() {
            callback(result);
        });
    })->start();
}

}

#endif
