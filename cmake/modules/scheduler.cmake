# scheduler.cmake — 定时任务调度模块 [v2.5.0]
# Layer: Extensions (E05) — 企业级扩展
# 依赖: core + di
# 职责: ScheduledTask/CronTrigger/Scheduler

add_library(soul_scheduler STATIC
    src/soul/scheduler/module.cpp
    src/soul/scheduler/scheduled_task.cpp
    src/soul/scheduler/scheduler.cpp
    # v3.0.0: Q_OBJECT 头文件必须加入 target sources 以便 AUTOMOC 扫描
    # 当 include/ 和 src/ 分离时，AUTOMOC 默认的同名匹配机制失效
    include/soul/scheduler/scheduled_task.h
    include/soul/scheduler/scheduler.h
)

target_include_directories(soul_scheduler PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_scheduler PUBLIC soul_core soul_di)