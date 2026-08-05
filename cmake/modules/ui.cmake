# ui.cmake — UI 组件模块 [v2.5.0]
# 依赖: core + base + configuration + Qt::Widgets + Qt::Gui

add_library(soul_ui STATIC
    src/soul/ui/animation.cpp
    src/soul/ui/avatar.cpp
    src/soul/ui/badge.cpp
    src/soul/ui/base_dialog.cpp
    src/soul/ui/base_view.cpp
    src/soul/ui/base_view_model.cpp
    src/soul/ui/base_widget.cpp
    src/soul/ui/button.cpp
    src/soul/ui/card.cpp
    src/soul/ui/checkbox.cpp
    src/soul/ui/dialog.cpp
    src/soul/ui/dropdown.cpp
    src/soul/ui/empty_widget.cpp
    src/soul/ui/glass_widget.cpp
    src/soul/ui/icon.cpp
    src/soul/ui/icon_manager.cpp
    src/soul/ui/input.cpp
    src/soul/ui/loading.cpp
    src/soul/ui/navigation.cpp
    src/soul/ui/page.cpp
    src/soul/ui/progress.cpp
    src/soul/ui/scroll_bar.cpp
    src/soul/ui/sidebar.cpp
    src/soul/ui/sidebar_hover_filter.cpp
    src/soul/ui/slider.cpp
    src/soul/ui/spinner.cpp
    src/soul/ui/style.cpp
    src/soul/ui/switch.cpp
    src/soul/ui/tab_bar.cpp
    src/soul/ui/theme.cpp
    src/soul/ui/toast.cpp
    src/soul/ui/tool_tip.cpp
    src/soul/ui/window.cpp
)

target_include_directories(soul_ui PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_ui PUBLIC
    soul_core
    soul_base
    soul_configuration
    Qt6::Widgets
    Qt6::Gui
)