# aop.cmake — 切面编程模块 [v2.5.0]
# 依赖: core + logging

add_library(soul_aop STATIC
    src/soul/aop/aop.cpp
)

target_include_directories(soul_aop PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_aop PUBLIC soul_core soul_logging)