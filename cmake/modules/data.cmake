# data.cmake — 数据访问抽象层模块 [v2.5.0]
# Layer: Infrastructure (I06) — 数据基础设施
# 依赖: core + Qt::Sql
# 职责: IRepository/BaseRepository/MemoryRepository/QueryBuilder/QueryCache/ORM Reflection/Transaction
#       MySQL(BS Server一等公民) / SQLite(CS Client本地数据)

add_library(soul_data STATIC
    src/soul/data/base_repository.cpp
    src/soul/data/database_driver.cpp
    src/soul/data/memory_repository.cpp
    src/soul/data/migration.cpp
    src/soul/data/orm_reflection.cpp
    src/soul/data/query_builder.cpp
    src/soul/data/query_cache.cpp
    src/soul/data/transaction.cpp
)

target_include_directories(soul_data PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_data PUBLIC soul_core Qt6::Sql)