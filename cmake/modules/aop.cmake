# aop.cmake — AOP 切面编程模块 [v2.5.0]
# Layer: Extensions (E07) — 企业级扩展
# 依赖: core + logging
# 职责: AspectWeaver/Before/After/Around/AfterReturning/AfterThrowing/Pointcut/JoinPoint

add_library(soul_aop STATIC
    src/soul/aop/aop.cpp
)

target_include_directories(soul_aop PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_aop PUBLIC soul_core soul_logging)