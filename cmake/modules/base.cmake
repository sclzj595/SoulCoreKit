# base.cmake — 基础类与抽象模块 [v2.5.0]
# Layer: Infrastructure (I11) — 通用基础设施
# 依赖: core + data
# 职责: BaseObject/BaseManager/BaseService/ILifecycle/INameable

add_library(soul_base STATIC
    src/soul/base/base_manager.cpp
    src/soul/base/base_object.cpp
    src/soul/base/base_service.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/base/base_manager.h
    include/soul/base/base_object.h
    include/soul/base/base_service.h
)

target_include_directories(soul_base PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_base PUBLIC soul_core soul_data)