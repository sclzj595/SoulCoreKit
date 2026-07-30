#ifndef SOUL_SCHEDULER_SCHEDULER_H
#define SOUL_SCHEDULER_SCHEDULER_H

#include <QObject>
#include <memory>
#include <vector>
#include <string>
#include <mutex>

#include "soul/scheduler/scheduled_task.h"

namespace sc {

// ============================================================================
// Scheduler — 定时任务调度器
// ============================================================================
//
// 管理多个 ScheduledTask,提供统一的启动/停止/查询接口。
// 对标 SpringBoot 的 @Scheduled 注解 + TaskScheduler。
//
// 用法:
//   Scheduler scheduler;
//   scheduler.addTask(ScheduledTask::createCron("0 0 * * *", []() { ... }));
//   scheduler.startAll();
//
// 线程安全: 所有公共方法均为线程安全。
class Scheduler : public QObject {
    Q_OBJECT
public:
    explicit Scheduler(QObject* parent = nullptr);
    ~Scheduler() override;

    // 禁止拷贝
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    // --- 任务管理 ---

    // 添加任务(自动去重,同名任务会替换旧任务)
    void addTask(std::shared_ptr<ScheduledTask> task);

    // 移除任务
    void removeTask(const std::string& name);

    // 按名称查找任务
    std::shared_ptr<ScheduledTask> findTask(const std::string& name) const;

    // 获取所有任务
    std::vector<std::shared_ptr<ScheduledTask>> allTasks() const;

    // 获取任务数量
    size_t taskCount() const;

    // --- 生命周期 ---

    // 启动所有任务
    void startAll();

    // 停止所有任务
    void stopAll();

    // 启动指定任务
    void startTask(const std::string& name);

    // 停止指定任务
    void stopTask(const std::string& name);

    // 是否所有任务已停止
    bool isAllStopped() const;

    // --- 统计 ---

    // 总执行次数
    int64_t totalExecutionCount() const;

signals:
    void taskAdded(const std::string& name);
    void taskRemoved(const std::string& name);
    void allStarted();
    void allStopped();

private:
    mutable std::mutex m_mutex;
    std::vector<std::shared_ptr<ScheduledTask>> m_tasks;
};

} // namespace sc

#endif // SOUL_SCHEDULER_SCHEDULER_H