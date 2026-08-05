# base.cmake — 基础类模块 [v2.5.0]
# 依赖: core + data

add_library(soul_base STATIC
    src/soul/base/base_manager.cpp
    src/soul/base/base_object.cpp
    src/soul/base/base_service.cpp
)

target_include_directories(soul_base PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_base PUBLIC soul_core soul_data)