# storage.cmake — KV 存储与持久化模块 [v3.0.0]
# Layer: Infrastructure (I09) — 数据基础设施
# 依赖: core + logging + Qt::Sql + nlohmann_json
# 职责: MemoryStorage/FileStorage/SQLiteDatabase/Settings/JsonSerializer
# v3.0.0: cache.cpp 已迁移到 soul_cache 模块

add_library(soul_storage STATIC
    src/soul/storage/file_storage.cpp
    src/soul/storage/json_serializer.cpp
    src/soul/storage/memory_storage.cpp
    src/soul/storage/settings.cpp
    src/soul/storage/sqlite_database.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/storage/settings.h
)

target_include_directories(soul_storage PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_storage PUBLIC soul_core soul_logging Qt6::Sql nlohmann_json::nlohmann_json PRIVATE spdlog::spdlog)