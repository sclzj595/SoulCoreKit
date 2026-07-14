#include "soul/async/cancelable_task.h"
#include <QThreadPool>

namespace sc {

CancelableTask::CancelableTask(QObject* parent) : QObject(parent) {
    setAutoDelete(false);
}

void CancelableTask::setTask(std::function<Result<void>()> task) {
    m_task = std::move(task);
}

void CancelableTask::setOnComplete(std::function<void(const Result<void>&)> callback) {
    m_onComplete = std::move(callback);
}

void CancelableTask::start() {
    m_cancelled = false;
    m_running = true;
    QThreadPool::globalInstance()->start(this);
}

void CancelableTask::cancel() {
    m_cancelled = true;
}

bool CancelableTask::isCancelled() const {
    return m_cancelled.load();
}

bool CancelableTask::isRunning() const {
    return m_running.load();
}

void CancelableTask::run() {
    if (m_cancelled) {
        m_running = false;
        return;
    }

    Result<void> result;
    if (m_task) {
        result = m_task();
    }

    m_running = false;

    if (m_onComplete) {
        m_onComplete(result);
    }

    emit completed(result);
}

}