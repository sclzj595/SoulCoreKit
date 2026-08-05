#ifndef SOUL_ASYNC_THREAD_POOL_H
#define SOUL_ASYNC_THREAD_POOL_H

#include <QThreadPool>
#include <QObject>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace sc {

// ============================================================================
// ThreadPool — 三级优先级线程池 [v1.9.2 重构]
// ============================================================================
//
// 设计目标: 实现三级优先级队列(High/Normal/Low) + 防饥饿机制,
// 确保低优先级任务在系统负载下不会被无限延迟。
//
// 优先级调度策略:
//   - High:   立即调度,抢占 Normal/Low
//   - Normal: 默认优先级,公平轮询
//   - Low:    低优先级,饥饿计数达到阈值后提升到 Normal
//
// 防饥饿机制:
//   - 每个 Low 任务携带 starvationCount
//   - 每次 Low 任务被跳过时 starvationCount++
//   - 达到 kStarvationThreshold(默认 10)后提升到 Normal 队列执行
//
// [v1.9.4 增强] 细粒度优先级通道 (PriorityTask + std::priority_queue):
//   - submitPriority(task, priority) 支持任意整数优先级(数值越大越优先)
//   - 与三级 Priority 枚举互补,适合需要超过 3 档优先级的场景
//   - 调度顺序: priority_queue 堆顶优先级 >= High 阈值时优先于 High 队列执行;
//              否则按 High → priority_queue → Normal → Low 顺序调度
//
// 用法:
//   ThreadPool::instance().init();
//   ThreadPool::instance().start(task, Priority::High);
//   ThreadPool::instance().start(task, Priority::Normal);
//   ThreadPool::instance().start(task, Priority::Low);
//   ThreadPool::instance().submitPriority(task, 100);  // [v1.9.4] 细粒度优先级

enum class Priority {
    High = 3,
    Normal = 2,
    Low = 1
};

// ============================================================================
// PriorityTask — 细粒度优先级任务 [v1.9.4]
// ============================================================================
//
// 携带任意整数优先级的任务包装类型,供 std::priority_queue 使用。
// 数值越大越优先;Comparator 构成 max-heap,堆顶为最高优先级任务。
//
// @thread_safety 非线程安全 — 仅作为值类型在 m_queueMutex 保护下入队/出队
struct PriorityTask {
    int priority = 0;                    ///< 优先级(数值越大越优先)
    std::function<void()> task;          ///< 任务函数

    /// @brief 优先级比较器(构成 max-heap,大值在堆顶)
    struct Comparator {
        bool operator()(const PriorityTask& a, const PriorityTask& b) const noexcept {
            return a.priority < b.priority;
        }
    };
};

class ThreadPool : public QObject {
    Q_OBJECT
public:
    static ThreadPool& instance();

    void init(int maxThreads = 0);
    void shutdown();
    bool isInitialized() const;

    /// @brief 提交任务(默认 Normal 优先级)
    void start(std::function<void()> task);

    /// @brief 提交任务(指定优先级) [v1.9.2 增强]
    void start(std::function<void()> task, int priority);

    /// @brief 提交任务(使用 Priority 枚举) [v1.9.2 新增]
    void start(std::function<void()> task, Priority priority);

    /// @brief 提交细粒度优先级任务(任意整数优先级) [v1.9.4 新增]
    /// @param task 任务函数
    /// @param priority 优先级(数值越大越优先,默认 0)
    /// @note 与三级 Priority 枚举互补,适合需要超过 3 档优先级的场景。
    ///       调度规则:
    ///         - priority >= High(3) 时,与 High 队列同档,堆顶最高者优先
    ///         - priority >= Normal(2) 时,在 High 队列清空后调度
    ///         - 其他情况在 Normal 队列清空后调度
    void submitPriority(std::function<void()> task, int priority = 0);

    int activeThreadCount() const;
    int maxThreadCount() const;
    void setMaxThreadCount(int maxThreads);

    int expiryTimeout() const;
    void setExpiryTimeout(int expiryTimeout);

    bool waitForDone(int msecs = -1);

    void reserveThread();
    void releaseThread();

    QThreadPool* nativeThreadPool();

    /// @brief 获取各优先级队列大小 [v1.9.2 新增]
    int queueSize() const;
    int highQueueSize() const;
    int normalQueueSize() const;
    int lowQueueSize() const;

    /// @brief 获取细粒度优先级队列大小 [v1.9.4 新增]
    int priorityQueueSize() const;

private:
    ThreadPool();
    ~ThreadPool() override;

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// @brief 工作线程主循环 [v1.9.2 新增]
    void workerLoop();

    /// @brief 从优先级队列中取任务(含防饥饿) [v1.9.2 新增]
    std::function<void()> dequeueTask();

    struct StarvationTask {
        std::function<void()> task;
        int starvationCount = 0;
    };

    static constexpr int kStarvationThreshold = 10;  ///< 饥饿提升阈值

    std::shared_ptr<QThreadPool> m_threadPool;
    std::atomic<bool> m_initialized{false};
    mutable std::mutex m_initMutex;

    // [v1.9.2] 三级优先级队列
    std::deque<std::function<void()>> m_highQueue;    ///< 高优先级队列
    std::deque<std::function<void()>> m_normalQueue;  ///< 普通优先级队列
    std::deque<StarvationTask> m_lowQueue;            ///< 低优先级队列(含饥饿计数)

    // [v1.9.4] 细粒度优先级队列(max-heap,堆顶为最高优先级任务)
    std::priority_queue<PriorityTask,
                        std::vector<PriorityTask>,
                        PriorityTask::Comparator> m_priorityQueue;

    mutable std::mutex m_queueMutex;
    std::condition_variable m_queueCv;                ///< 队列非空通知
    std::condition_variable m_doneCv;                 ///< [v1.9.4] 任务完成通知(替代 waitForDone 忙等)
    std::vector<std::thread> m_workers;               ///< 工作线程池
    std::atomic<bool> m_running{false};               ///< 线程池运行标志
    std::atomic<int> m_activeCount{0};                ///< 活跃线程计数
    std::atomic<int> m_maxWorkers{0};                 ///< 最大工作线程数
};

}

#endif