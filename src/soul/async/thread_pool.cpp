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
    if (m_initialized) {
        return;
    }
    m_threadPool = std::make_unique<QThreadPool>();
    if (maxThreads > 0) {
        m_threadPool->setMaxThreadCount(maxThreads);
    } else {
        m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
    }
    m_initialized = true;
    SingletonRegistry::instance().registerShutdown([this]() {
        shutdown();
    });
}

void ThreadPool::shutdown() {
    if (!m_initialized || !m_threadPool) {
        return;
    }
    m_threadPool->waitForDone();
    m_threadPool.reset();
    m_initialized = false;
}

bool ThreadPool::isInitialized() const {
    return m_initialized;
}

void ThreadPool::start(std::function<void()> task) {
    if (!m_initialized) {
        init();
    }
    m_threadPool->start(std::move(task));
}

void ThreadPool::start(std::function<void()> task, int priority) {
    if (!m_initialized) {
        init();
    }
    m_threadPool->start(std::move(task), priority);
}

int ThreadPool::activeThreadCount() const {
    if (!m_initialized) {
        return 0;
    }
    return m_threadPool->activeThreadCount();
}

int ThreadPool::maxThreadCount() const {
    if (!m_initialized) {
        return QThread::idealThreadCount();
    }
    return m_threadPool->maxThreadCount();
}

void ThreadPool::setMaxThreadCount(int maxThreads) {
    if (!m_initialized) {
        init(maxThreads);
        return;
    }
    m_threadPool->setMaxThreadCount(maxThreads);
}

int ThreadPool::expiryTimeout() const {
    if (!m_initialized) {
        return 30000;
    }
    return m_threadPool->expiryTimeout();
}

void ThreadPool::setExpiryTimeout(int expiryTimeout) {
    if (!m_initialized) {
        init();
    }
    m_threadPool->setExpiryTimeout(expiryTimeout);
}

bool ThreadPool::waitForDone(int msecs) {
    if (!m_initialized) {
        return true;
    }
    return m_threadPool->waitForDone(msecs);
}

void ThreadPool::reserveThread() {
    if (!m_initialized) {
        init();
    }
    m_threadPool->reserveThread();
}

void ThreadPool::releaseThread() {
    if (!m_initialized) {
        init();
    }
    m_threadPool->releaseThread();
}

QThreadPool* ThreadPool::nativeThreadPool() {
    if (!m_initialized) {
        init();
    }
    return m_threadPool.get();
}

ThreadPool::~ThreadPool() {
    shutdown();
}

}