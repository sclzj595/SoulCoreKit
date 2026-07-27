#ifndef SOUL_ASYNC_TASK_RUNNER_H
#define SOUL_ASYNC_TASK_RUNNER_H

#include <QObject>
#include <exception>
#include <functional>
#include <memory>
#include <atomic>
#include <type_traits>
#include "soul/core/result.h"
#include "soul/async/future.h"
#include "soul/async/future_detail.h"

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

        m_activeTasks.fetch_add(1);
        QThreadPool::globalInstance()->start([this, func = std::forward<F>(func), promise = std::move(promise)]() mutable {
            try {
                if constexpr (std::is_void_v<ResultType>) {
                    func();
                    promise.finish();
                } else {
                    promise.addResult(func());
                    promise.finish();
                }
            } catch (const std::exception&) {
                promise.setException(std::current_exception());
                promise.finish();
            } catch (...) {
                // Blanket catch: run() task translates unknown exceptions into failed promise.
                promise.setException(std::current_exception());
                promise.finish();
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
            } catch (const std::exception& e) {
                detail::logAsyncException("TaskRunner::runAsync task", e.what());
            } catch (...) {
                // Blanket catch: runAsync task must not propagate exceptions to thread pool.
                detail::logAsyncUnknownException("TaskRunner::runAsync task");
            }
            m_activeTasks.fetch_sub(1);
        });
    }

    int activeTaskCount() const;
    Result<void> waitForAll(int msecs = -1);

signals:
    void taskStarted();
    void taskFinished();

private:
    std::atomic<int> m_activeTasks{0};
};

}

#endif
