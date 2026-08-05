#ifndef SOUL_SERVER_SCHEDULEDTASKS_ENDPOINT_H
#define SOUL_SERVER_SCHEDULEDTASKS_ENDPOINT_H

// ============================================================================
// scheduledtasks_endpoint.h — 定时任务内省端点 [v1.9.4]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/scheduledtasks,暴露 Scheduler 中所有
// 定时任务的调度配置与执行统计(模式/间隔/cron 表达式/执行次数/运行状态)。
//
// 依赖 sc::Scheduler 提供的 allTasks() 查询接口。由于 allTasks() 返回
// shared_ptr<ScheduledTask>,需要 ScheduledTask 的完整类型,故本端点直接
// 包含 scheduler.h 与 scheduled_task.h,并在头文件中实现。
//
// 用法:
//   server.get("/actuator/scheduledtasks", [&scheduler](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(ScheduledTasksEndpoint::toJson(scheduler));
//   });

#include "soul/utils/json/json_helper.h"
#include "soul/scheduler/scheduler.h"
#include "soul/scheduler/scheduled_task.h"

#include <QByteArray>
#include <cstdint>

namespace sc {
namespace server {

// ============================================================================
// ScheduledTasksEndpoint — 定时任务内省端点
// ============================================================================
class ScheduledTasksEndpoint {
public:
    /// @brief 从 Scheduler 导出所有定时任务为 JSON
    /// @param scheduler Scheduler 实例(用于获取任务列表)
    /// @return JSON 格式的定时任务列表(对标 SpringBoot /actuator/scheduledtasks)
    static QByteArray toJson(const sc::Scheduler& scheduler) {
        // allTasks() 为 const 成员方法,可直接在 const 引用上调用
        auto tasks = scheduler.allTasks();

        sc::json::Json root = sc::json::Json::object();
        sc::json::Json taskList = sc::json::Json::array();

        // 遍历所有任务,构建条目
        for (const auto& task : tasks) {
            sc::json::Json entry = sc::json::Json::object();
            entry["name"] = task->name();

            // 调度模式转换: Cron/FixedRate/FixedDelay → 字符串
            std::string modeStr;
            switch (task->mode()) {
                case ScheduledTask::Mode::Cron:       modeStr = "cron";        break;
                case ScheduledTask::Mode::FixedRate:  modeStr = "fixedRate";   break;
                case ScheduledTask::Mode::FixedDelay: modeStr = "fixedDelay";  break;
            }
            entry["mode"] = modeStr;
            entry["intervalMs"] = task->intervalMs();
            entry["cron"] = task->cronExpression();
            entry["executionCount"] = task->executionCount();
            entry["running"] = task->isRunning();
            taskList.push_back(entry);
        }

        root["scheduledTasks"] = taskList;
        root["total"] = tasks.size();
        return sc::json::serializePretty(root);
    }
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_SCHEDULEDTASKS_ENDPOINT_H
