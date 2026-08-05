# data.cmake — 数据访问抽象层模块 [v2.5.0]
# 依赖: core

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
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_data PUBLIC soul_core)