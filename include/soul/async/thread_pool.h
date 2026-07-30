#ifndef SOUL_ASYNC_THREAD_POOL_H
#define SOUL_ASYNC_THREAD_POOL_H

#include <QThreadPool>
#include <QObject>
#include <QTimer>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <deque>
#include <condition_variable>
#include <thread>
#include <chrono>

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
// 用法:
//   ThreadPool::instance().init();
//   ThreadPool::instance().start(task, Priority::High);
//   ThreadPool::instance().start(task, Priority::Normal);
//   ThreadPool::instance().start(task, Priority::Low);

enum class Priority {
    High = 3,
    Normal = 2,
    Low = 1
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

    mutable std::mutex m_queueMutex;
    std::condition_variable m_queueCv;                ///< 队列非空通知
    std::vector<std::thread> m_workers;               ///< 工作线程池
    std::atomic<bool> m_running{false};               ///< 线程池运行标志
    std::atomic<int> m_activeCount{0};                ///< 活跃线程计数
    std::atomic<int> m_maxWorkers{0};                 ///< 最大工作线程数
};

}

#endif