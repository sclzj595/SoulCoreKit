#include "soul/async/task_runner.h"
#include "soul/core/error.h"

namespace sc {

TaskRunner::TaskRunner(QObject* parent)
    : QObject(parent) {
}

int TaskRunner::activeTaskCount() const {
    return m_activeTasks.load();
}

Result<void> TaskRunner::waitForAll(int msecs) {
    if (!QThreadPool::globalInstance()->waitForDone(msecs)) {
        return Error(ErrorCode::Timeout, "waitForAll timed out");
    }
    return {};
}

}
