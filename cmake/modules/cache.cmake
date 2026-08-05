# cache.cmake — 缓存系统模块 [v2.5.0]
# 依赖: core + logging (PRIVATE)

add_library(soul_cache STATIC
    src/soul/cache/disk_cache.cpp
)

target_include_directories(soul_cache PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_cache PUBLIC soul_core)
target_link_libraries(soul_cache PRIVATE soul_logging)