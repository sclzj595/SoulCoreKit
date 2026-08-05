# scheduler.cmake — 定时任务模块 [v2.5.0]
# 依赖: core + di

add_library(soul_scheduler STATIC
    src/soul/scheduler/module.cpp
    src/soul/scheduler/scheduled_task.cpp
    src/soul/scheduler/scheduler.cpp
)

target_include_directories(soul_scheduler PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_scheduler PUBLIC soul_core soul_di)