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
    if (m_initialized.load(std::memory_order_acquire)) {
        return;
    }
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
    SingletonRegistry::instance().registerShutdown([this]() {
        shutdown();
    });
}

void ThreadPool::shutdown() {
    std::shared_ptr<QThreadPool> pool;
    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        if (!m_initialized.load(std::memory_order_relaxed) || !m_threadPool) {
            return;
        }
        pool = m_threadPool;
    }
    if (pool) {
        pool->waitForDone();
    }
    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        m_threadPool.reset();
        m_initialized.store(false, std::memory_order_relaxed);
    }
}

bool ThreadPool::isInitialized() const {
    return m_initialized.load(std::memory_order_acquire);
}

void ThreadPool::start(std::function<void()> task) {
    if (!m_initialized.load(std::memory_order_acquire)) {
        init();
    }
    auto pool = m_threadPool;
    if (pool) {
        pool->start(std::move(task));
    }
}

void ThreadPool::start(std::function<void()> task, int priority) {
    if (!m_initialized.load(std::memory_order_acquire)) {
        init();
    }
    auto pool = m_threadPool;
    if (pool) {
        pool->start(std::move(task), priority);
    }
}

int ThreadPool::activeThreadCount() const {
    if (!m_initialized.load(std::memory_order_acquire)) {
        return 0;
    }
    auto pool = m_threadPool;
    return pool ? pool->activeThreadCount() : 0;
}

int ThreadPool::maxThreadCount() const {
    if (!m_initialized.load(std::memory_order_acquire)) {
        return QThread::idealThreadCount();
    }
    auto pool = m_threadPool;
    return pool ? pool->maxThreadCount() : QThread::idealThreadCount();
}

void ThreadPool::setMaxThreadCount(int maxThreads) {
    if (!m_initialized.load(std::memory_order_acquire)) {
        init(maxThreads);
        return;
    }
    auto pool = m_threadPool;
    if (pool) {
        pool->setMaxThreadCount(maxThreads);
    }
}

int ThreadPool::expiryTimeout() const {
    if (!m_initialized.load(std::memory_order_acquire)) {
        return 30000;
    }
    auto pool = m_threadPool;
    return pool ? pool->expiryTimeout() : 30000;
}

void ThreadPool::setExpiryTimeout(int expiryTimeout) {
    if (!m_initialized.load(std::memory_order_acquire)) {
        init();
    }
    auto pool = m_threadPool;
    if (pool) {
        pool->setExpiryTimeout(expiryTimeout);
    }
}

bool ThreadPool::waitForDone(int msecs) {
    if (!m_initialized.load(std::memory_order_acquire)) {
        return true;
    }
    auto pool = m_threadPool;
    return pool ? pool->waitForDone(msecs) : true;
}

void ThreadPool::reserveThread() {
    if (!m_initialized.load(std::memory_order_acquire)) {
        init();
    }
    auto pool = m_threadPool;
    if (pool) {
        pool->reserveThread();
    }
}

void ThreadPool::releaseThread() {
    if (!m_initialized.load(std::memory_order_acquire)) {
        init();
    }
    auto pool = m_threadPool;
    if (pool) {
        pool->releaseThread();
    }
}

QThreadPool* ThreadPool::nativeThreadPool() {
    if (!m_initialized.load(std::memory_order_acquire)) {
        init();
    }
    auto pool = m_threadPool;
    return pool ? pool.get() : nullptr;
}

ThreadPool::~ThreadPool() {
    shutdown();
}

}