#include "soul/async/thread_pool.h"
#include <functional>
#include "soul/core/singleton.h"

namespace sc {

ThreadPool::ThreadPool()
    : m_threadPool(std::make_unique<QThreadPool>()) {
    m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
    SingletonRegistry::instance().registerShutdown([this]() {
        m_threadPool->waitForDone();
    });
}

ThreadPool& ThreadPool::instance() {
    static ThreadPool instance;
    return instance;
}

void ThreadPool::start(std::function<void()> task) {
    m_threadPool->start(std::move(task));
}

void ThreadPool::start(std::function<void()> task, int priority) {
    m_threadPool->start(std::move(task), priority);
}

int ThreadPool::activeThreadCount() const {
    return m_threadPool->activeThreadCount();
}

int ThreadPool::maxThreadCount() const {
    return m_threadPool->maxThreadCount();
}

void ThreadPool::setMaxThreadCount(int maxThreads) {
    m_threadPool->setMaxThreadCount(maxThreads);
}

int ThreadPool::expiryTimeout() const {
    return m_threadPool->expiryTimeout();
}

void ThreadPool::setExpiryTimeout(int expiryTimeout) {
    m_threadPool->setExpiryTimeout(expiryTimeout);
}

bool ThreadPool::waitForDone(int msecs) {
    return m_threadPool->waitForDone(msecs);
}

void ThreadPool::reserveThread() {
    m_threadPool->reserveThread();
}

void ThreadPool::releaseThread() {
    m_threadPool->releaseThread();
}

QThreadPool* ThreadPool::nativeThreadPool() {
    return m_threadPool.get();
}

}