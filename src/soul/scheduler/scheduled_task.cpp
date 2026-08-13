#include "soul/scheduler/scheduled_task.h"

#include <QCoreApplication>
#include <sstream>
#include <algorithm>

namespace sc {

// ============================================================================
// ScheduledTask
// ============================================================================

ScheduledTask::ScheduledTask(Mode mode, TaskCallback callback, const std::string& name)
    : QObject(nullptr)
    , m_mode(mode)
    , m_callback(std::move(callback))
    , m_name(name)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    QObject::connect(m_timer, &QTimer::timeout, this, &ScheduledTask::onTimerTick);
}

std::shared_ptr<ScheduledTask> ScheduledTask::createCron(
    const std::string& cronExpression,
    TaskCallback callback,
    const std::string& name)
{
    auto task = std::shared_ptr<ScheduledTask>(
        new ScheduledTask(Mode::Cron, std::move(callback), name),
        [](ScheduledTask* p) { p->deleteLater(); }
    );
    task->m_cronExpression = cronExpression;
    return task;
}

std::shared_ptr<ScheduledTask> ScheduledTask::createFixedRate(
    int intervalMs,
    TaskCallback callback,
    const std::string& name)
{
    auto task = std::shared_ptr<ScheduledTask>(
        new ScheduledTask(Mode::FixedRate, std::move(callback), name),
        [](ScheduledTask* p) { p->deleteLater(); }
    );
    task->m_intervalMs = intervalMs;
    return task;
}

std::shared_ptr<ScheduledTask> ScheduledTask::createFixedDelay(
    int intervalMs,
    TaskCallback callback,
    const std::string& name)
{
    auto task = std::shared_ptr<ScheduledTask>(
        new ScheduledTask(Mode::FixedDelay, std::move(callback), name),
        [](ScheduledTask* p) { p->deleteLater(); }
    );
    task->m_intervalMs = intervalMs;
    return task;
}

void ScheduledTask::start()
{
    if (m_running.exchange(true)) {
        return; // 已经启动
    }

    emit started();
    scheduleNext();
}

void ScheduledTask::stop()
{
    m_running.store(false);
    m_timer->stop();
    emit stopped();
}

void ScheduledTask::triggerNow()
{
    if (m_executing.exchange(true)) {
        return; // 已在执行中
    }

    try {
        if (m_callback) {
            m_callback();
        }
    } catch (...) { // Blanket catch: rethrow to propagate task execution failure
        m_executing.store(false);
        throw;
    }

    m_executionCount.fetch_add(1);
    m_lastExecutionTime = QDateTime::currentDateTime();
    m_executing.store(false);

    emit executed();
}

bool ScheduledTask::isRunning() const
{
    return m_running.load();
}

QDateTime ScheduledTask::lastExecutionTime() const
{
    return m_lastExecutionTime;
}

void ScheduledTask::onTimerTick()
{
    if (!m_running.load()) {
        return;
    }

    triggerNow();

    if (m_running.load()) {
        scheduleNext();
    }
}

void ScheduledTask::scheduleNext()
{
    int nextIntervalMs = 0;

    switch (m_mode) {
    case Mode::Cron: {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime next = nextCronTime(now);
        if (!next.isValid()) {
            return; // 解析失败,不调度
        }
        qint64 msecs = now.msecsTo(next);
        nextIntervalMs = static_cast<int>(std::max<qint64>(100, msecs));
        break;
    }
    case Mode::FixedRate:
    case Mode::FixedDelay:
        nextIntervalMs = m_intervalMs;
        break;
    }

    m_timer->start(nextIntervalMs);
}

// ============================================================================
// Cron 表达式解析
// ============================================================================
// 标准 5 字段格式: minute hour day-of-month month day-of-week
// 支持: * , - / 和数字
// 示例: "0 */5 * * *" = 每5分钟
//       "0 0 * * *"   = 每天0点
//       "0 8 * * 1-5" = 工作日8点

QDateTime ScheduledTask::nextCronTime(const QDateTime& from) const
{
    // 解析 cron 字段
    std::istringstream iss(m_cronExpression);
    std::string minuteF, hourF, domF, monthF, dowF;
    iss >> minuteF >> hourF >> domF >> monthF >> dowF;

    if (iss.fail()) {
        return QDateTime(); // 解析失败
    }

    QDateTime candidate = from;
    candidate = candidate.addSecs(60); // 从下一秒开始
    candidate.setTime(QTime(candidate.time().hour(), candidate.time().minute(), 0, 0));

    // 最多搜索 2 年(约 1051200 分钟),避免死循环
    const int maxIterations = 1051200;
    for (int i = 0; i < maxIterations; ++i) {
        QDate d = candidate.date();
        QTime t = candidate.time();

        int month = d.month();
        int day = d.day();
        int dayOfWeek = d.dayOfWeek() % 7; // Qt: 1=Mon → 0=Mon, 6=Sun
        int hour = t.hour();
        int minute = t.minute();

        if (matchCronField(minute, minuteF, 0, 59) &&
            matchCronField(hour, hourF, 0, 23) &&
            matchCronField(day, domF, 1, 31) &&
            matchCronField(month, monthF, 1, 12) &&
            matchCronField(dayOfWeek, dowF, 0, 6)) {
            return candidate;
        }

        candidate = candidate.addSecs(60);
    }

    return QDateTime(); // 未找到匹配
}

bool ScheduledTask::matchCronField(int value, const std::string& field, int min, int max) const
{
    if (field.empty()) {
        return true;
    }

    // 处理 * 通配符
    if (field == "*") {
        return true;
    }

    // 处理逗号分隔的列表
    std::istringstream iss(field);
    std::string part;
    while (std::getline(iss, part, ',')) {
        // 处理 / 步进
        size_t slashPos = part.find('/');
        int step = 1;
        std::string rangePart = part;
        if (slashPos != std::string::npos) {
            step = std::stoi(part.substr(slashPos + 1));
            rangePart = part.substr(0, slashPos);
        }

        // 处理 - 范围
        size_t dashPos = rangePart.find('-');
        int rangeMin, rangeMax;
        if (dashPos != std::string::npos) {
            rangeMin = std::stoi(rangePart.substr(0, dashPos));
            rangeMax = std::stoi(rangePart.substr(dashPos + 1));
        } else if (rangePart == "*") {
            rangeMin = min;
            rangeMax = max;
        } else {
            rangeMin = std::stoi(rangePart);
            rangeMax = rangeMin;
        }

        // 检查值是否在范围内且匹配步进
        if (value >= rangeMin && value <= rangeMax) {
            if (step == 1 || (value - rangeMin) % step == 0) {
                return true;
            }
        }
    }

    return false;
}

} // namespace sc
