#include "soul/scheduler/module.h"
#include "soul/scheduler/scheduler.h"
#include "soul/di/container.h"

namespace sc {

SchedulerModule::SchedulerModule()
    : Module("scheduler")
{
}

Result<void> SchedulerModule::init()
{
    auto scheduler = std::make_shared<Scheduler>();
    auto bindResult = di::Container::instance().bindSingleton<Scheduler>(
        [scheduler]() -> Scheduler* { return scheduler.get(); }
    );
    if (!bindResult.isOk()) {
        return bindResult;
    }
    return {};
}

Result<void> SchedulerModule::onStart()
{
    // 调度器在此启动,确保所有依赖模块已初始化
    // 具体任务由业务模块通过 DI 获取 Scheduler 后自行添加
    return {};
}

void SchedulerModule::onStop()
{
    auto result = di::Container::instance().resolve<Scheduler>();
    if (result.isOk()) {
        auto scheduler = result.unwrap();
        if (scheduler) {
            scheduler->stopAll();
        }
    }
}

void SchedulerModule::cleanup()
{
}

} // namespace sc