#include "soul/async/thread_pool.h"
#include <functional>
#include "soul/core/singleton.h"

namespace sc {

ThreadPool::ThreadPool() {
}

ThreadPool& ThreadPool::instance() {
    static ThreadPool instance;
    return instance;
}

void ThreadPool::init(int maxThreads) {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (m_initialized.load(std::memory_order_relaxed)) {
        return;
    }
    m_threadPool = std::make_shared<QThreadPool>();
    if (maxThreads > 0) {
        m_threadPool->setMaxThreadCount(maxThreads);
        m_maxWorkers.store(maxThreads, std::memory_order_relaxed);
    } else {
        int ideal = QThread::idealThreadCount();
        m_threadPool->setMaxThreadCount(ideal);
        m_maxWorkers.store(ideal, std::memory_order_relaxed);
    }
    m_initialized.store(true, std::memory_order_release);

    // [v1.9.2] 启动工作线程(三级优先级调度)
    m_running.store(true, std::memory_order_release);
    int workerCount = m_maxWorkers.load(std::memory_order_relaxed);
    m_workers.reserve(workerCount);
    for (int i = 0; i < workerCount; ++i) {
        m_workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

void ThreadPool::shutdown() {
    // [v1.9.2] 停止工作线程
    // TSan-safe: 持 m_queueMutex 保护 m_workers 的读写,避免与 start()/waitForDone() 的竞争
    m_running.store(false, std::memory_order_release);
    m_queueCv.notify_all();
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        m_workers.clear();
    }

    std::shared_ptr<QThreadPool> pool;
    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        if (!m_initialized.load(std::memory_order_relaxed) || !m_threadPool) {
            return;
        }
        pool = std::move(m_threadPool);
        m_initialized.store(false, std::memory_order_relaxed);
    }
    if (pool) {
        pool->waitForDone();
    }
}

bool ThreadPool::isInitialized() const {
    return m_initialized.load(std::memory_order_acquire);
}

// ============================================================================
// 三级优先级任务提交 [v1.9.2]
// ============================================================================

void ThreadPool::start(std::function<void()> task) {
    start(std::move(task), Priority::Normal);
}

void ThreadPool::start(std::function<void()> task, int priority) {
    switch (priority) {
    case static_cast<int>(Priority::High):
        start(std::move(task), Priority::High);
        break;
    case static_cast<int>(Priority::Low):
        start(std::move(task), Priority::Low);
        break;
    default:
        start(std::move(task), Priority::Normal);
        break;
    }
}

void ThreadPool::start(std::function<void()> task, Priority priority) {
    if (!task) return;

    // [v1.9.2] 回退: 若工作线程未启动(init() 未调用),使用 QThreadPool 直接执行
    // TSan-safe: 持 m_queueMutex 读取 m_workers,避免与 init()/shutdown() 的写竞争
    bool fallback = false;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        fallback = m_workers.empty();
    }
    if (fallback) {
        std::lock_guard<std::mutex> lock(m_initMutex);
        if (!m_threadPool) {
            m_threadPool = std::make_shared<QThreadPool>();
            m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
            m_initialized.store(true, std::memory_order_release);
        }
        m_threadPool->start(std::move(task));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        switch (priority) {
        case Priority::High:
            m_highQueue.push_back(std::move(task));
            break;
        case Priority::Normal:
            m_normalQueue.push_back(std::move(task));
            break;
        case Priority::Low:
            m_lowQueue.push_back({std::move(task), 0});
            break;
        }
    }
    m_queueCv.notify_one();
}

// ============================================================================
// 工作线程循环 [v1.9.2]
// ============================================================================

void ThreadPool::workerLoop() {
    while (m_running.load(std::memory_order_acquire)) {
        auto task = dequeueTask();
        if (task) {
            m_activeCount.fetch_add(1, std::memory_order_relaxed);
            task();
            m_activeCount.fetch_sub(1, std::memory_order_relaxed);
        }
    }
}

std::function<void()> ThreadPool::dequeueTask() {
    std::unique_lock<std::mutex> lock(m_queueMutex);

    // 等待任务到达
    m_queueCv.wait(lock, [this]() {
        return !m_running.load(std::memory_order_acquire) ||
               !m_highQueue.empty() ||
               !m_normalQueue.empty() ||
               !m_lowQueue.empty();
    });

    if (!m_running.load(std::memory_order_acquire)) {
        return nullptr;
    }

    // 1. 优先处理 High 队列
    if (!m_highQueue.empty()) {
        auto task = std::move(m_highQueue.front());
        m_highQueue.pop_front();
        return task;
    }

    // 2. 处理 Normal 队列
    if (!m_normalQueue.empty()) {
        auto task = std::move(m_normalQueue.front());
        m_normalQueue.pop_front();
        return task;
    }

    // 3. 处理 Low 队列(含防饥饿)
    if (!m_lowQueue.empty()) {
        auto& st = m_lowQueue.front();
        st.starvationCount++;

        // 饥饿计数达到阈值:提升到 Normal 队列执行
        if (st.starvationCount >= kStarvationThreshold) {
            auto task = std::move(st.task);
            m_lowQueue.pop_front();
            return task;
        }

        // 否则调度下一个 Low 任务(轮询避免饥饿)
        auto task = std::move(st.task);
        m_lowQueue.pop_front();
        return task;
    }

    return nullptr;
}

// ============================================================================
// 队列大小查询 [v1.9.2]
// ============================================================================

int ThreadPool::queueSize() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return static_cast<int>(m_highQueue.size() + m_normalQueue.size() + m_lowQueue.size());
}

int ThreadPool::highQueueSize() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return static_cast<int>(m_highQueue.size());
}

int ThreadPool::normalQueueSize() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return static_cast<int>(m_normalQueue.size());
}

int ThreadPool::lowQueueSize() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return static_cast<int>(m_lowQueue.size());
}

int ThreadPool::activeThreadCount() const {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_initialized.load(std::memory_order_relaxed)) {
        return 0;
    }
    return m_threadPool ? m_threadPool->activeThreadCount() : 0;
}

int ThreadPool::maxThreadCount() const {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_initialized.load(std::memory_order_relaxed)) {
        return QThread::idealThreadCount();
    }
    return m_threadPool ? m_threadPool->maxThreadCount() : QThread::idealThreadCount();
}

void ThreadPool::setMaxThreadCount(int maxThreads) {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_initialized.load(std::memory_order_relaxed)) {
        if (!m_threadPool) {
            m_threadPool = std::make_shared<QThreadPool>();
        }
        m_threadPool->setMaxThreadCount(maxThreads);
        m_initialized.store(true, std::memory_order_release);
        return;
    }
    if (m_threadPool) {
        m_threadPool->setMaxThreadCount(maxThreads);
    }
}

int ThreadPool::expiryTimeout() const {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_initialized.load(std::memory_order_relaxed)) {
        return 30000;
    }
    return m_threadPool ? m_threadPool->expiryTimeout() : 30000;
}

void ThreadPool::setExpiryTimeout(int expiryTimeout) {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_initialized.load(std::memory_order_relaxed)) {
        if (!m_threadPool) {
            m_threadPool = std::make_shared<QThreadPool>();
            m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
            m_initialized.store(true, std::memory_order_release);
        }
    }
    if (m_threadPool) {
        m_threadPool->setExpiryTimeout(expiryTimeout);
    }
}

bool ThreadPool::waitForDone(int msecs) {
    // [v1.9.2] 若使用优先级队列,等待队列清空 + 活跃任务完成
    // TSan-safe: 持 m_queueMutex 读取 m_workers,避免与 init()/shutdown() 的竞争
    bool usePriorityWorkers = false;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        usePriorityWorkers = !m_workers.empty();
    }
    if (usePriorityWorkers) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(msecs);
        while (true) {
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (m_highQueue.empty() && m_normalQueue.empty() && m_lowQueue.empty()
                    && m_activeCount.load(std::memory_order_relaxed) == 0) {
                    return true;
                }
            }
            if (msecs >= 0 && std::chrono::steady_clock::now() >= deadline) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // 回退: 使用 QThreadPool
    std::shared_ptr<QThreadPool> pool;
    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        if (!m_initialized.load(std::memory_order_relaxed)) {
            return true;
        }
        pool = m_threadPool;
    }
    return pool ? pool->waitForDone(msecs) : true;
}

void ThreadPool::reserveThread() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_initialized.load(std::memory_order_relaxed)) {
        if (!m_threadPool) {
            m_threadPool = std::make_shared<QThreadPool>();
            m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
            m_initialized.store(true, std::memory_order_release);
        }
    }
    if (m_threadPool) {
        m_threadPool->reserveThread();
    }
}

void ThreadPool::releaseThread() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_initialized.load(std::memory_order_relaxed)) {
        return;
    }
    if (m_threadPool) {
        m_threadPool->releaseThread();
    }
}

QThreadPool* ThreadPool::nativeThreadPool() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_initialized.load(std::memory_order_relaxed)) {
        if (!m_threadPool) {
            m_threadPool = std::make_shared<QThreadPool>();
            m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
            m_initialized.store(true, std::memory_order_release);
        }
    }
    return m_threadPool ? m_threadPool.get() : nullptr;
}

ThreadPool::~ThreadPool() {
    shutdown();
}

}
