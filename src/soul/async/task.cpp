#include "soul/async/task.h"

namespace sc {

Task::Task() : m_cancelled(false) {
}

Task::Task(const Work& work) : m_work(work), m_cancelled(false) {
}

void Task::setWork(const Work& work) {
    m_work = work;
}

void Task::setCallback(const Callback& callback) {
    m_callback = callback;
}

void Task::run() {
    if (m_cancelled) return;
    if (m_work) {
        m_work();
    }
    if (m_callback) {
        m_callback(!m_cancelled);
    }
}

void Task::cancel() {
    m_cancelled = true;
}

bool Task::isCancelled() const {
    return m_cancelled;
}

}