# orm.cmake — ORM 对象关系映射模块 [v2.5.0]
# Layer: Infrastructure (I08) — 数据基础设施
# 依赖: core + logging + data + cache + Qt::Sql
# 职责: QueryWrapper/SqlDialect(MySQL/SQLite/PostgreSQL)/MigrationManager/CachedRepository/SC_REFLECT反射宏

add_library(soul_orm STATIC
    src/soul/orm/migration.cpp
    src/soul/orm/module.cpp
    src/soul/orm/query_wrapper.cpp
    src/soul/orm/sql_dialect.cpp
)

target_include_directories(soul_orm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_orm PUBLIC soul_core soul_logging soul_data soul_cache Qt6::Sql)