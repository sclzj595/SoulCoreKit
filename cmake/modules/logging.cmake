# logging.cmake — 日志系统模块 [v2.5.0]
# 依赖: core + spdlog

add_library(soul_logging STATIC
    src/soul/logging/callback_sink.cpp
    src/soul/logging/composite_sink.cpp
    src/soul/logging/console_sink.cpp
    src/soul/logging/daily_file_sink.cpp
    src/soul/logging/file_sink.cpp
    src/soul/logging/logger.cpp
    src/soul/logging/log_formatter.cpp
)

target_include_directories(soul_logging PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_logging PUBLIC soul_core spdlog::spdlog)