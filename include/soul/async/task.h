#ifndef SOUL_ASYNC_TASK_H
#define SOUL_ASYNC_TASK_H

#include <functional>
#include <atomic>
#include <memory>

namespace sc {

/**
 * @class Task
 * @brief 异步任务封装类
 *
 * Task 封装了一个可执行的工作单元，支持：
 * - 设置工作函数
 * - 设置完成回调
 * - 取消任务
 * - 检查任务状态
 *
 * 使用方式：
 * @code
 * Task task([](){
 *     // 执行耗时操作
 * });
 * task.setCallback([](bool success){
 *     // 处理完成
 * });
 * task.run();
 * @endcode
 *
 * @see AsyncRunner
 */
class Task {
public:
    /**
     * @brief 工作函数类型
     */
    using Work = std::function<void()>;

    /**
     * @brief 完成回调类型
     */
    using Callback = std::function<void(bool)>;

    /**
     * @brief 默认构造函数
     */
    Task();

    /**
     * @brief 构造函数（带工作函数）
     * @param work 工作函数
     */
    Task(const Work& work);

    /**
     * @brief 设置工作函数
     * @param work 工作函数
     */
    void setWork(const Work& work);

    /**
     * @brief 设置完成回调
     * @param callback 完成回调函数
     */
    void setCallback(const Callback& callback);

    /**
     * @brief 执行任务
     */
    void run();

    /**
     * @brief 取消任务
     */
    void cancel();

    /**
     * @brief 检查任务是否已取消
     * @return 如果已取消返回 true
     */
    bool isCancelled() const;

private:
    Work m_work;
    Callback m_callback;
    std::atomic<bool> m_cancelled;
};

/**
 * @brief Task 智能指针类型
 */
using TaskPtr = std::shared_ptr<Task>;

}

#endif
