# logging.cmake — 日志系统模块 [v2.5.0]
# Layer: Core (C03) — CS/BS 共用极稳定核心
# 依赖: core + spdlog
# 职责: Logger/ISink/FileSink/DailyFileSink/ConsoleSink/CompositeSink/CallbackSink

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
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_logging PUBLIC soul_core spdlog::spdlog)