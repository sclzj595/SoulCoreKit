# ui.cmake — 声明式 UI 组件库模块 [v2.5.0]
# Layer: Application (A01) — CS Client 专用
# 依赖: core + base + configuration + Qt::Widgets + Qt::Gui
# 职责: 30+ Widgets(Button/Card/Dialog/Toast/Nav/TabBar等) + Theme主题切换 + QSS样式 + iOS17玻璃效果
#       仅 CS Qt Desktop Client 使用，BS/Headless 项目不链接此模块

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
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/ui/avatar.h
    include/soul/ui/badge.h
    include/soul/ui/base_dialog.h
    include/soul/ui/base_view.h
    include/soul/ui/base_view_model.h
    include/soul/ui/base_widget.h
    include/soul/ui/button.h
    include/soul/ui/card.h
    include/soul/ui/checkbox.h
    include/soul/ui/dialog.h
    include/soul/ui/dropdown.h
    include/soul/ui/empty_widget.h
    include/soul/ui/glass_widget.h
    include/soul/ui/icon_manager.h
    include/soul/ui/input.h
    include/soul/ui/loading.h
    include/soul/ui/page.h
    include/soul/ui/progress.h
    include/soul/ui/scroll_bar.h
    include/soul/ui/sidebar.h
    include/soul/ui/sidebar_hover_filter.h
    include/soul/ui/slider.h
    include/soul/ui/spinner.h
    include/soul/ui/switch.h
    include/soul/ui/tab_bar.h
    include/soul/ui/theme.h
    include/soul/ui/toast.h
    include/soul/ui/tool_tip.h
    include/soul/ui/window.h
)

target_include_directories(soul_ui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_ui PUBLIC
    soul_core
    soul_base
    soul_configuration
    Qt6::Widgets
    Qt6::Gui
)