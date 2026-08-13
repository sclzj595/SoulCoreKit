# cs.cmake — CS 架构核心模块 [v2.5.0]
# Layer: Application (A02) — CS 架构适配
# 依赖: core + di + ui + data + application
# 职责: CsController/CsRouter/CsViewModel/CsWindowManager/CsAdminPanel/CsIpcRouter
#       CsDataBinding/CsDialogManager/CsErrorHandler/CsFormValidator/CsNavigation/CsService
#       仅完整 CS 应用使用，BS/Headless 项目不链接此模块

add_library(soul_cs STATIC
    src/soul/cs/cs_admin_panel.cpp
    src/soul/cs/cs_controller.cpp
    src/soul/cs/cs_data_binding.cpp
    src/soul/cs/cs_dialog_manager.cpp
    src/soul/cs/cs_error_handler.cpp
    src/soul/cs/cs_form_validator.cpp
    src/soul/cs/cs_ipc_router.cpp
    src/soul/cs/cs_module.cpp
    src/soul/cs/cs_navigation.cpp
    src/soul/cs/cs_router.cpp
    src/soul/cs/cs_service.cpp
    src/soul/cs/cs_view_model.cpp
    src/soul/cs/cs_window_manager.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/cs/cs_admin_panel.h
    include/soul/cs/cs_controller.h
    include/soul/cs/cs_data_binding.h
    include/soul/cs/cs_dialog_manager.h
    include/soul/cs/cs_error_handler.h
    include/soul/cs/cs_ipc_router.h
    include/soul/cs/cs_navigation.h
    include/soul/cs/cs_router.h
    include/soul/cs/cs_service.h
    include/soul/cs/cs_view_model.h
    include/soul/cs/cs_window_manager.h
)

target_include_directories(soul_cs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_cs PUBLIC
    soul_core
    soul_di
    soul_ui
    soul_data
    soul_application
    Qt6::Widgets
    Qt6::Network
)