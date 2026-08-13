# async.cmake — 异步任务框架模块 [v2.5.0]
# Layer: Core (C04) — CS/BS 共用极稳定核心
# 依赖: core + logging (PRIVATE)
# 职责: ThreadPool/TaskRunner/Future/Promise/Coroutine/Dispatcher/CancelableTask

add_library(soul_async STATIC
    src/soul/async/async_runner.cpp
    src/soul/async/cancelable_task.cpp
    src/soul/async/dispatcher.cpp
    src/soul/async/future_detail.cpp
    src/soul/async/task.cpp
    src/soul/async/task_runner.cpp
    src/soul/async/thread_pool.cpp
    # v3.0.0: Q_OBJECT 头文件必须加入 target sources 以便 AUTOMOC 扫描
    # 当 include/ 和 src/ 分离时，AUTOMOC 默认的同名匹配机制失效
    include/soul/async/cancelable_task.h
    include/soul/async/task_runner.h
    include/soul/async/thread_pool.h
)

set_target_properties(soul_async PROPERTIES AUTOMOC ON)

target_include_directories(soul_async PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_async PUBLIC soul_core)
target_link_libraries(soul_async PRIVATE soul_logging)