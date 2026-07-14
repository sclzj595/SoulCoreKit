#ifndef SOUL_ASYNC_THREAD_POOL_H
#define SOUL_ASYNC_THREAD_POOL_H

#include <QThreadPool>
#include <QObject>
#include <memory>
#include <atomic>

namespace sc {

class ThreadPool : public QObject {
    Q_OBJECT
public:
    static ThreadPool& instance();

    void start(std::function<void()> task);
    void start(std::function<void()> task, int priority);

    int activeThreadCount() const;
    int maxThreadCount() const;
    void setMaxThreadCount(int maxThreads);

    int expiryTimeout() const;
    void setExpiryTimeout(int expiryTimeout);

    bool waitForDone(int msecs = -1);

    void reserveThread();
    void releaseThread();

    QThreadPool* nativeThreadPool();

signals:
    void threadStarted();
    void threadFinished();

private:
    ThreadPool();
    ~ThreadPool() override = default;

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    std::unique_ptr<QThreadPool> m_threadPool;
};

}

#endif
