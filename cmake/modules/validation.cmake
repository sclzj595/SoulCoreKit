# validation.cmake — 输入验证模块 [v2.5.0]
# 依赖: core

add_library(soul_validation STATIC
    src/soul/validation/validator.cpp
)

target_include_directories(soul_validation PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_validation PUBLIC soul_core)