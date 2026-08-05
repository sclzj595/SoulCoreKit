# async.cmake — 异步任务模块 [v2.5.0]
# 依赖: core + logging (PRIVATE)

add_library(soul_async STATIC
    src/soul/async/async_runner.cpp
    src/soul/async/cancelable_task.cpp
    src/soul/async/dispatcher.cpp
    src/soul/async/future_detail.cpp
    src/soul/async/task.cpp
    src/soul/async/task_runner.cpp
    src/soul/async/thread_pool.cpp
)

target_include_directories(soul_async PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_async PUBLIC soul_core)
target_link_libraries(soul_async PRIVATE soul_logging)