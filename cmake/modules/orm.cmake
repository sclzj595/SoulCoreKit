# orm.cmake — ORM 映射模块 [v2.5.0]
# 依赖: core + logging + data + cache

add_library(soul_orm STATIC
    src/soul/orm/migration.cpp
    src/soul/orm/module.cpp
    src/soul/orm/query_wrapper.cpp
    src/soul/orm/sql_dialect.cpp
)

target_include_directories(soul_orm PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_orm PUBLIC soul_core soul_logging soul_data soul_cache)