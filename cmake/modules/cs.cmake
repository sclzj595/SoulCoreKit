# cs.cmake — CS 架构核心模块 [v2.5.0]
# 依赖: core + di + ui + data + application

add_library(soul_cs STATIC
    src/soul/cs/cs_controller.cpp
    src/soul/cs/cs_data_binding.cpp
    src/soul/cs/cs_dialog_manager.cpp
    src/soul/cs/cs_error_handler.cpp
    src/soul/cs/cs_form_validator.cpp
    src/soul/cs/cs_module.cpp
    src/soul/cs/cs_navigation.cpp
    src/soul/cs/cs_router.cpp
    src/soul/cs/cs_service.cpp
    src/soul/cs/cs_view_model.cpp
    src/soul/cs/cs_window_manager.cpp
)

target_include_directories(soul_cs PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_cs PUBLIC
    soul_core
    soul_di
    soul_ui
    soul_data
    soul_application
)