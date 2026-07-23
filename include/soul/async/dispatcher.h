#ifndef SOUL_ASYNC_DISPATCHER_H
#define SOUL_ASYNC_DISPATCHER_H

#include <QThread>
#include <QThreadPool>
#include <functional>
#include <memory>

namespace sc {

/**
 * @class Dispatcher
 * @brief 线程调度器，提供跨线程任务调度功能
 *
 * Dispatcher 提供统一的线程调度接口：
 * - async()：在后台线程执行任务
 * - dispatchToMain()：将任务调度到主线程执行
 * - dispatchToMainDelayed()：延迟调度到主线程
 * - runOnWorker()：在工作线程执行任务
 *
 * 使用方式：
 * @code
 * // 在后台线程执行
 * Dispatcher::async([](){
 *     // 耗时操作
 * });
 *
 * // 切换到主线程更新 UI
 * Dispatcher::dispatchToMain([](){
 *     // 更新 UI
 * });
 * @endcode
 */
class Dispatcher {
public:
    /**
     * @brief 在后台线程执行任务（默认优先级）
     * @param task 要执行的任务
     */
    static void async(std::function<void()> task);

    /**
     * @brief 在后台线程执行任务（指定优先级）
     * @param task 要执行的任务
     * @param priority 任务优先级
     */
    static void async(std::function<void()> task, int priority);

    /**
     * @brief 将任务调度到主线程执行
     * @param task 要执行的任务
     */
    static void dispatchToMain(std::function<void()> task);

    /**
     * @brief 延迟将任务调度到主线程执行
     * @param task 要执行的任务
     * @param delayMs 延迟时间（毫秒）
     */
    static void dispatchToMainDelayed(std::function<void()> task, int delayMs);

    /**
     * @brief 在工作线程执行任务（默认优先级）
     * @param task 要执行的任务
     */
    static void runOnWorker(std::function<void()> task);

    /**
     * @brief 在工作线程执行任务（指定优先级）
     * @param task 要执行的任务
     * @param priority 任务优先级
     */
    static void runOnWorker(std::function<void()> task, int priority);

    /**
     * @brief 设置最大线程数
     * @param maxThreads 最大线程数
     */
    static void setMaxThreads(int maxThreads);

    /**
     * @brief 获取最大线程数
     * @return 最大线程数
     */
    static int maxThreads();

private:
    /**
     * @brief 获取线程池实例
     * @return QThreadPool 引用
     */
    static QThreadPool& threadPool();
};

}

#endif
