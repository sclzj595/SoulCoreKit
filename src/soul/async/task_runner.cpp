#include "soul/async/task_runner.h"

namespace sc {

TaskRunner::TaskRunner(QObject* parent)
    : QObject(parent) {
}

int TaskRunner::activeTaskCount() const {
    return m_activeTasks.load();
}

bool TaskRunner::waitForAll(int msecs) {
    return QThreadPool::globalInstance()->waitForDone(msecs);
}

}
