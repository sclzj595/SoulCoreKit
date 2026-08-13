# cache.cmake — 缓存系统模块 [v2.9.2]
# Layer: Infrastructure (I10) — 数据基础设施
# 依赖: core + logging (PRIVATE)
# 职责: ICache/MemoryCache/DiskCache/MultiLevelCache/RedisCache/SizeEstimator

add_library(soul_cache STATIC
    src/soul/cache/disk_cache.cpp
    src/soul/cache/redis_cache.cpp  # v2.9.2
)

target_include_directories(soul_cache PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_cache PUBLIC soul_core)
target_link_libraries(soul_cache PRIVATE soul_logging)

# v2.9.2: Redis Adapter (可选)
option(SOULCOREKIT_ENABLE_REDIS "Enable Redis cache backend (requires hiredis)" OFF)
if(SOULCOREKIT_ENABLE_REDIS)
    find_package(hiredis QUIET)
    if(hiredis_FOUND)
        target_compile_definitions(soul_cache PUBLIC SOUL_ENABLE_REDIS=1)
        target_link_libraries(soul_cache PRIVATE hiredis::hiredis)
        message(STATUS "Redis cache backend enabled (hiredis)")
    else()
        message(WARNING "SOULCOREKIT_ENABLE_REDIS=ON but hiredis not found. "
                        "RedisCache will be a stub.")
    endif()
endif()
