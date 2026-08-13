# database.cmake — 数据库实现聚合模块 [v2.5.0]
# Layer: Infrastructure (I07) — 数据基础设施
# 依赖: data + Qt::Sql
# 职责: IDatabaseDriver/ConnectionPool/MigrationManager
#       MySQL 一等公民 (BS Server) / SQLite (CS Client Local Cache)
# [审计] INTERFACE 库: 纯依赖聚合无源文件, 避免空静态库 ar 警告。

add_library(soul_database INTERFACE)

target_include_directories(soul_database INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_database INTERFACE soul_data Qt6::Sql)