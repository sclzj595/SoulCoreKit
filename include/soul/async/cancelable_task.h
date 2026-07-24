#ifndef SOUL_ASYNC_CANCELABLE_TASK_H
#define SOUL_ASYNC_CANCELABLE_TASK_H

#include <QObject>
#include <QRunnable>
#include <atomic>
#include <functional>
#include "soul/core/result.h"

namespace sc {

class CancelableTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    explicit CancelableTask(QObject* parent = nullptr);
    ~CancelableTask() override = default;

    void setTask(std::function<Result<void>()> task);
    void setOnComplete(std::function<void(const Result<void>&)> callback);

    void start();
    void cancel();

    bool isCancelled() const;
    bool isRunning() const;

signals:
    void completed(const Result<void>& result);
    void progress(int percent);

protected:
    void run() override;

private:
    std::function<Result<void>()> m_task;
    std::function<void(const Result<void>&)> m_onComplete;
    std::atomic<bool> m_cancelled{false};
    std::atomic<bool> m_running{false};
};

}

#endif