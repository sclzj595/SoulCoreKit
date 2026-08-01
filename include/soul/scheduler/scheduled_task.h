#ifndef SOUL_SCHEDULER_SCHEDULED_TASK_H
#define SOUL_SCHEDULER_SCHEDULED_TASK_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <functional>
#include <string>
#include <atomic>
#include <memory>

namespace sc {

// ============================================================================
// ScheduledTask — 定时任务基类
// ============================================================================
//
// 支持三种调度模式:
//   - Cron: 基于 cron 表达式,在匹配的时间点触发
//   - FixedRate: 固定频率,从上一次开始执行的时间点计算间隔
//   - FixedDelay: 固定延迟,从上一次执行完成后计算间隔
//
// 用法:
//   auto task = ScheduledTask::createCron("0 */5 * * *", []() { ... });
//   task->start();
//
// 线程安全: 所有公共方法均为线程安全,可在任意线程调用。
class ScheduledTask : public QObject {
    Q_OBJECT
public:
    using TaskCallback = std::function<void()>;

    enum class Mode {
        Cron,
        FixedRate,
        FixedDelay
    };

    // --- 工厂方法 ---

    // 创建 cron 任务
    static std::shared_ptr<ScheduledTask> createCron(
        const std::string& cronExpression,
        TaskCallback callback,
        const std::string& name = ""
    );

    // 创建固定频率任务
    static std::shared_ptr<ScheduledTask> createFixedRate(
        int intervalMs,
        TaskCallback callback,
        const std::string& name = ""
    );

    // 创建固定延迟任务
    static std::shared_ptr<ScheduledTask> createFixedDelay(
        int intervalMs,
        TaskCallback callback,
        const std::string& name = ""
    );

    // --- 生命周期 ---

    // 启动任务调度
    void start();

    // 停止任务调度(等待当前执行完成)
    void stop();

    // 立即触发一次执行(不影响调度周期)
    void triggerNow();

    // --- 状态查询 ---

    bool isRunning() const;
    Mode mode() const { return m_mode; }
    const std::string& name() const { return m_name; }
    const std::string& cronExpression() const { return m_cronExpression; }
    int intervalMs() const { return m_intervalMs; }

    // 执行统计
    int64_t executionCount() const { return m_executionCount.load(); }
    QDateTime lastExecutionTime() const;

signals:
    void started();
    void stopped();
    void executed();

private:
    explicit ScheduledTask(Mode mode, TaskCallback callback, const std::string& name);

    void onTimerTick();
    void scheduleNext();
    QDateTime nextCronTime(const QDateTime& from) const;
    bool matchCronField(int value, const std::string& field, int min, int max) const;

    Mode m_mode;
    TaskCallback m_callback;
    std::string m_name;
    std::string m_cronExpression;
    int m_intervalMs = 0;

    QTimer* m_timer = nullptr;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_executing{false};
    std::atomic<int64_t> m_executionCount{0};
    mutable QDateTime m_lastExecutionTime;
};

} // namespace sc

#endif // SOUL_SCHEDULER_SCHEDULED_TASK_H