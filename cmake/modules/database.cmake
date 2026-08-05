# database.cmake — 数据库实现层模块 [v2.5.0]
# 依赖: data + Qt::Sql

add_library(soul_database STATIC
)

target_include_directories(soul_database PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_database PUBLIC soul_data Qt6::Sql)