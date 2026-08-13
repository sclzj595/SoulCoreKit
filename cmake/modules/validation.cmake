# validation.cmake — 数据校验模块 [v2.5.0]
# Layer: Infrastructure (I15) — 通用基础设施
# 依赖: core
# 职责: Validator/注解式校验/规则链

add_library(soul_validation STATIC
    src/soul/validation/validator.cpp
)

target_include_directories(soul_validation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_validation PUBLIC soul_core)