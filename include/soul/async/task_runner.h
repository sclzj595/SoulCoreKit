#ifndef SOUL_ASYNC_TASK_RUNNER_H
#define SOUL_ASYNC_TASK_RUNNER_H

#include <QObject>
#include <functional>
#include <memory>
#include <atomic>
#include "soul/async/future.h"

namespace sc {

class TaskRunner : public QObject {
    Q_OBJECT
public:
    explicit TaskRunner(QObject* parent = nullptr);
    ~TaskRunner() override = default;

    template<typename F>
    Future<std::invoke_result_t<F>> run(F&& func) {
        using ResultType = std::invoke_result_t<F>;

        QPromise<ResultType> promise;
        auto future = promise.future();

        auto task = std::make_shared<std::packaged_task<ResultType()>>(std::forward<F>(func));
        auto futureResult = task->get_future();

        m_activeTasks.fetch_add(1);
        QThreadPool::globalInstance()->start([this, task = std::move(task), promise = std::move(promise)]() mutable {
            try {
                auto result = (*task)();
                promise.addResult(std::move(result));
                promise.finish();
            } catch (...) {
                promise.setException(std::current_exception());
            }
            m_activeTasks.fetch_sub(1);
        });

        return Future<ResultType>(std::move(future));
    }

    template<typename F>
    void runAsync(F&& func) {
        m_activeTasks.fetch_add(1);
        QThreadPool::globalInstance()->start([this, func = std::forward<F>(func)]() {
            try {
                func();
            } catch (...) {
            }
            m_activeTasks.fetch_sub(1);
        });
    }

    int activeTaskCount() const;
    bool waitForAll(int msecs = -1);

signals:
    void taskStarted();
    void taskFinished();

private:
    std::atomic<int> m_activeTasks{0};
};

}

#endif
