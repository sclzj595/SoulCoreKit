#ifndef SOUL_ASYNC_THREAD_POOL_H
#define SOUL_ASYNC_THREAD_POOL_H

#include <QThreadPool>
#include <QObject>
#include <memory>

namespace sc {

class ThreadPool : public QObject {
    Q_OBJECT
public:
    static ThreadPool& instance();

    void init(int maxThreads = 0);
    void shutdown();
    bool isInitialized() const;

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

private:
    ThreadPool();
    ~ThreadPool() override;

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    std::unique_ptr<QThreadPool> m_threadPool;
    bool m_initialized = false;
};

}

#endif