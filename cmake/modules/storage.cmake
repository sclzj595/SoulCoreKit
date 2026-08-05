# storage.cmake — 存储模块 [v2.5.0]
# 依赖: core + Qt::Sql

add_library(soul_storage STATIC
    src/soul/storage/cache.cpp
    src/soul/storage/file_storage.cpp
    src/soul/storage/json_serializer.cpp
    src/soul/storage/memory_storage.cpp
    src/soul/storage/settings.cpp
    src/soul/storage/sqlite_database.cpp
)

target_include_directories(soul_storage PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_storage PUBLIC soul_core Qt6::Sql)