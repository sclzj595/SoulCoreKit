find_package(Qt6 REQUIRED COMPONENTS Test Core Network)

# ============================================================================
# 测试辅助函数 — 消除样板代码，每个测试从 5 行压缩为 1 行
# ============================================================================

set(SOUL_TEST_ONE_VALUE_ARGS "")
set(SOUL_TEST_MULTI_VALUE_ARGS SOUL_LIBS QT_LIBS)

# soul_add_test — 标准测试(无 UI 依赖)
# 用法: soul_add_test(test_xxx SOUL_LIBS soul_xxx soul_yyy [QT_LIBS Qt6::Network Qt6::Sql])
function(soul_add_test TEST_NAME)
    cmake_parse_arguments(ARG "" "${SOUL_TEST_ONE_VALUE_ARGS}" "${SOUL_TEST_MULTI_VALUE_ARGS}" ${ARGN})

    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Test
        Qt6::Core
        ${ARG_QT_LIBS}
        ${ARG_SOUL_LIBS}
    )
    windeployqt_post_build(${TEST_NAME} TRUE)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# soul_add_test_ui — UI 测试(自动条件链接 Qt6::Widgets)
# 用法: soul_add_test_ui(test_ui SOUL_LIBS soul_ui soul_core soul_base soul_configuration)
function(soul_add_test_ui TEST_NAME)
    cmake_parse_arguments(ARG "" "${SOUL_TEST_ONE_VALUE_ARGS}" "${SOUL_TEST_MULTI_VALUE_ARGS}" ${ARGN})

    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Test
        Qt6::Core
        ${ARG_QT_LIBS}
        ${ARG_SOUL_LIBS}
    )
    if(TARGET Qt6::Widgets)
        target_link_libraries(${TEST_NAME} PRIVATE Qt6::Widgets)
    endif()
    windeployqt_post_build(${TEST_NAME} TRUE)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# soul_add_test_widgets — Widgets 专属测试(整体在 if(TARGET Qt6::Widgets) 内)
# 用法: soul_add_test_widgets(test_ui_button SOUL_LIBS soul_core soul_ui soul_logging)
function(soul_add_test_widgets TEST_NAME)
    cmake_parse_arguments(ARG "" "${SOUL_TEST_ONE_VALUE_ARGS}" "${SOUL_TEST_MULTI_VALUE_ARGS}" ${ARGN})

    if(TARGET Qt6::Widgets)
        add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
        target_link_libraries(${TEST_NAME} PRIVATE
            Qt6::Test
            Qt6::Widgets
            Qt6::Core
            Qt6::Gui
            ${ARG_QT_LIBS}
            ${ARG_SOUL_LIBS}
        )
        windeployqt_post_build(${TEST_NAME} TRUE)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endif()
endfunction()

# soul_add_test_cxx20 — C++20 协程测试(整体在 if(ENABLE_CXX20) 内)
# 用法: soul_add_test_cxx20(test_coroutine SOUL_LIBS soul_async soul_core)
function(soul_add_test_cxx20 TEST_NAME)
    cmake_parse_arguments(ARG "" "${SOUL_TEST_ONE_VALUE_ARGS}" "${SOUL_TEST_MULTI_VALUE_ARGS}" ${ARGN})

    if(ENABLE_CXX20)
        add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
        target_link_libraries(${TEST_NAME} PRIVATE
            Qt6::Test
            Qt6::Core
            ${ARG_QT_LIBS}
            ${ARG_SOUL_LIBS}
        )
        windeployqt_post_build(${TEST_NAME} TRUE)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endif()
endfunction()

# ============================================================================
# Foundation 层测试
# ============================================================================

# --- Core ---
soul_add_test(test_result           SOUL_LIBS soul_core)
soul_add_test(test_core             SOUL_LIBS soul_core)
soul_add_test(test_module_registry  SOUL_LIBS soul_core soul_logging)

# --- DI ---
soul_add_test(test_di               SOUL_LIBS soul_di soul_core)

# --- Logging ---
soul_add_test(test_logger           SOUL_LIBS soul_logging soul_core)

# --- Data ---
soul_add_test(test_data             SOUL_LIBS soul_database soul_data soul_core       QT_LIBS Qt6::Sql)
soul_add_test(test_repository_factory SOUL_LIBS soul_database soul_core soul_data soul_orm  QT_LIBS Qt6::Sql)

# --- Base (uses UI, needs conditional Widgets) ---
soul_add_test_ui(test_base          SOUL_LIBS soul_base soul_auth soul_network soul_storage
                                             soul_utils soul_configuration soul_logging
                                             soul_core soul_data soul_ui
                                     QT_LIBS Qt6::Network Qt6::Sql)

# --- Utils ---
soul_add_test(test_utils            SOUL_LIBS soul_utils soul_core      QT_LIBS Qt6::Network Qt6::Xml)

# --- Configuration ---
soul_add_test(test_configuration    SOUL_LIBS soul_configuration soul_core)
soul_add_test(test_config_bind      SOUL_LIBS soul_configuration soul_core)
soul_add_test(test_remote_config    SOUL_LIBS soul_configuration soul_core  QT_LIBS Qt6::Network)

# --- Storage ---
soul_add_test(test_storage          SOUL_LIBS soul_storage soul_core)

# --- Cache ---
soul_add_test(test_cache            SOUL_LIBS soul_storage soul_core)
soul_add_test(test_soul_cache       SOUL_LIBS soul_cache soul_core soul_logging)

# --- Async ---
soul_add_test(test_async            SOUL_LIBS soul_async soul_core)

# --- Event ---
soul_add_test(test_event_bus        SOUL_LIBS soul_event soul_core)
soul_add_test(test_typed_event_bus  SOUL_LIBS soul_event soul_core soul_async soul_logging)

# --- Network ---
soul_add_test(test_http_request     SOUL_LIBS soul_network soul_core)
soul_add_test(test_network          SOUL_LIBS soul_network soul_core soul_logging  QT_LIBS Qt6::Network)
soul_add_test(test_connection_manager SOUL_LIBS soul_network soul_event soul_core soul_logging  QT_LIBS Qt6::Network)

# --- Auth ---
soul_add_test(test_auth             SOUL_LIBS soul_auth soul_network soul_storage soul_utils
                                             soul_base soul_configuration soul_logging soul_core
                                     QT_LIBS Qt6::Network Qt6::Sql)
soul_add_test(test_oauth2           SOUL_LIBS soul_auth soul_base soul_core  QT_LIBS Qt6::Network)

# --- MQ ---
soul_add_test(test_mq               SOUL_LIBS soul_mq soul_core soul_logging soul_async  QT_LIBS Qt6::Network)

# --- ORM ---
soul_add_test(test_orm              SOUL_LIBS soul_orm soul_database soul_data soul_core soul_logging  QT_LIBS Qt6::Sql)

# --- Observability ---
soul_add_test(test_observability    SOUL_LIBS soul_observability soul_logging soul_core)
soul_add_test(test_resource_pool_monitor SOUL_LIBS soul_observability soul_async soul_network
                                                   soul_database soul_data soul_core soul_logging
                                                   QT_LIBS Qt6::Network Qt6::Sql)

# --- AOP ---
soul_add_test(test_aop              SOUL_LIBS soul_aop soul_core soul_logging)

# --- Validation ---
# (test_v193_components 覆盖了 Validator)

# --- Scheduler ---
soul_add_test(test_scheduler        SOUL_LIBS soul_scheduler soul_core soul_di)

# --- Server ---
soul_add_test(test_http_server      SOUL_LIBS soul_server soul_core soul_logging  QT_LIBS Qt6::Network)
soul_add_test(test_health           SOUL_LIBS soul_server soul_core soul_logging)
soul_add_test(test_websocket_server SOUL_LIBS soul_server soul_core soul_logging  QT_LIBS Qt6::Network Qt6::WebSockets)

# --- RPC ---
soul_add_test(test_rpc              SOUL_LIBS soul_rpc soul_core        QT_LIBS Qt6::Network)

# --- Plugin ---
soul_add_test(test_plugin           SOUL_LIBS soul_plugin soul_di soul_core)

# ============================================================================
# Application 层测试
# ============================================================================

# --- UI 组件(条件链接 Widgets) ---
soul_add_test_ui(test_ui            SOUL_LIBS soul_ui soul_core soul_base soul_configuration)
soul_add_test_ui(test_ui_switch     SOUL_LIBS soul_ui soul_core soul_base soul_configuration)
soul_add_test_ui(test_ui_avatar     SOUL_LIBS soul_ui soul_core soul_base soul_configuration)
soul_add_test_ui(test_ui_dropdown   SOUL_LIBS soul_ui soul_core soul_base soul_configuration)
soul_add_test_ui(test_ui_window     SOUL_LIBS soul_ui soul_core soul_base soul_configuration)

# --- UI 组件(Widgets 专属) ---
soul_add_test_widgets(test_ui_button    SOUL_LIBS soul_core soul_ui soul_logging)
soul_add_test_widgets(test_ui_components1 SOUL_LIBS soul_core soul_ui soul_logging)  # GlassWidget/Animation/IconManager
soul_add_test_widgets(test_ui_components2 SOUL_LIBS soul_core soul_ui soul_logging)  # BaseDialog/BaseViewModel
soul_add_test_widgets(test_ui_components3 SOUL_LIBS soul_core soul_ui soul_logging)  # Checkbox/Card/Badge/Input/Progress
soul_add_test_widgets(test_ui_components4 SOUL_LIBS soul_core soul_ui soul_logging)  # Slider/Spinner/TabBar/Toast/ToolTip
soul_add_test_widgets(test_ui_components5 SOUL_LIBS soul_core soul_ui soul_logging)  # Loading/EmptyWidget/Page/SideBar/Navigation
soul_add_test_widgets(test_ui_components6 SOUL_LIBS soul_core soul_ui soul_logging)  # BaseWidget/BaseView/Icon/ScrollBar

# ============================================================================
# 版本综合测试
# ============================================================================

# v1.9.3: CircuitBreaker / RateLimiter / Validator / InfoEndpoint / LoggersEndpoint
soul_add_test(test_v193_components  SOUL_LIBS soul_server soul_network soul_validation
                                              soul_observability soul_core soul_logging
                                     QT_LIBS Qt6::Network)

# v1.9.4: Metrics/ThreadDump/Beans/Caches/ScheduledTasks/Shutdown
#         + ThreadPool PriorityTask + ConnectionPool 动态扩缩容 + OtlpExporter
soul_add_test(test_v194_components  SOUL_LIBS soul_server soul_network soul_async
                                              soul_observability soul_scheduler soul_di
                                              soul_core soul_logging
                                     QT_LIBS Qt6::Network)

# ============================================================================
# 条件编译测试
# ============================================================================

# v1.9.1: C++20 coroutine support tests (GAP-15)
soul_add_test_cxx20(test_coroutine  SOUL_LIBS soul_async soul_core)