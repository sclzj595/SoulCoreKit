#include "soul/scheduler/scheduler.h"

#include <algorithm>

namespace sc {

Scheduler::Scheduler(QObject* parent)
    : QObject(parent)
{
}

Scheduler::~Scheduler()
{
    stopAll();
}

void Scheduler::addTask(std::shared_ptr<ScheduledTask> task)
{
    if (!task) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 同名任务替换
    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
        [&](const auto& t) { return t->name() == task->name(); });

    if (it != m_tasks.end()) {
        (*it)->stop();
        *it = task;
    } else {
        m_tasks.push_back(task);
    }

    emit taskAdded(task->name());
}

void Scheduler::removeTask(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
        [&](const auto& t) { return t->name() == name; });

    if (it != m_tasks.end()) {
        (*it)->stop();
        m_tasks.erase(it);
        emit taskRemoved(name);
    }
}

std::shared_ptr<ScheduledTask> Scheduler::findTask(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
        [&](const auto& t) { return t->name() == name; });

    if (it != m_tasks.end()) {
        return *it;
    }
    return nullptr;
}

std::vector<std::shared_ptr<ScheduledTask>> Scheduler::allTasks() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks;
}

size_t Scheduler::taskCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks.size();
}

void Scheduler::startAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& task : m_tasks) {
        task->start();
    }
    emit allStarted();
}

void Scheduler::stopAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& task : m_tasks) {
        task->stop();
    }
    emit allStopped();
}

void Scheduler::startTask(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
        [&](const auto& t) { return t->name() == name; });
    if (it != m_tasks.end()) {
        (*it)->start();
    }
}

void Scheduler::stopTask(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
        [&](const auto& t) { return t->name() == name; });
    if (it != m_tasks.end()) {
        (*it)->stop();
    }
}

bool Scheduler::isAllStopped() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::all_of(m_tasks.begin(), m_tasks.end(),
        [](const auto& t) { return !t->isRunning(); });
}

int64_t Scheduler::totalExecutionCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int64_t total = 0;
    for (const auto& task : m_tasks) {
        total += task->executionCount();
    }
    return total;
}

} // namespace sc
