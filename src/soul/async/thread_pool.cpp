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
    } else {
        m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
    }
    m_initialized.store(true, std::memory_order_release);
}

void ThreadPool::shutdown() {
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

void ThreadPool::start(std::function<void()> task) {
    std::shared_ptr<QThreadPool> pool;
    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        if (!m_initialized.load(std::memory_order_relaxed)) {
            if (!m_threadPool) {
                m_threadPool = std::make_shared<QThreadPool>();
                m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
                m_initialized.store(true, std::memory_order_release);
            }
        }
        pool = m_threadPool;
    }
    if (pool) {
        pool->start(std::move(task));
    }
}

void ThreadPool::start(std::function<void()> task, int priority) {
    std::shared_ptr<QThreadPool> pool;
    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        if (!m_initialized.load(std::memory_order_relaxed)) {
            if (!m_threadPool) {
                m_threadPool = std::make_shared<QThreadPool>();
                m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
                m_initialized.store(true, std::memory_order_release);
            }
        }
        pool = m_threadPool;
    }
    if (pool) {
        pool->start(std::move(task), priority);
    }
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
