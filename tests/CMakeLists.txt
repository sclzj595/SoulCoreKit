# ============================================================================
# SoulCoreKit Tests — v2.6.0 测试分层体系
# ============================================================================
# 测试分层:
#   unit/         — 单元测试: 单个类/函数的隔离测试
#   integration/  — 集成测试: 多模块协作测试
#   lifecycle/    — 生命周期测试: ILifecycle 语义、状态转换
#   concurrency/  — 并发测试: 线程安全、竞态条件
#   contract/     — 契约测试: API 兼容性、ABI 稳定性
#
# 运行:
#   ctest                          # 全部测试
#   ctest -L unit                  # 仅单元测试
#   ctest -L lifecycle             # 仅生命周期测试
#   ctest -L concurrency           # 仅并发测试 (TSan 推荐)
# ============================================================================

find_package(Qt6 REQUIRED COMPONENTS Test Core Network)

# ============================================================================
# 测试辅助函数
# ============================================================================

set(SOUL_TEST_ONE_VALUE_ARGS "")
set(SOUL_TEST_MULTI_VALUE_ARGS SOUL_LIBS QT_LIBS LABELS)

# soul_add_test — 标准测试 (无 UI 依赖)
# 用法: soul_add_test(test_xxx [SUBDIR subdir] SOUL_LIBS soul_xxx soul_yyy [QT_LIBS Qt6::Network] [LABELS unit])
function(soul_add_test TEST_NAME)
    cmake_parse_arguments(ARG "" "SUBDIR" "${SOUL_TEST_MULTI_VALUE_ARGS}" ${ARGN})

    if(ARG_SUBDIR)
        set(SRC "${ARG_SUBDIR}/${TEST_NAME}.cpp")
    else()
        # Auto-detect subdirectory: unit/ or integration/ or tests/ root
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit/${TEST_NAME}.cpp")
            set(SRC "unit/${TEST_NAME}.cpp")
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integration/${TEST_NAME}.cpp")
            set(SRC "integration/${TEST_NAME}.cpp")
        else()
            set(SRC "${TEST_NAME}.cpp")
        endif()
    endif()

    add_executable(${TEST_NAME} ${SRC})
    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Test
        Qt6::Core
        ${ARG_QT_LIBS}
        ${ARG_SOUL_LIBS}
    )
    windeployqt_post_build(${TEST_NAME} TRUE)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    if(ARG_LABELS)
        set_tests_properties(${TEST_NAME} PROPERTIES LABELS "${ARG_LABELS}")
    endif()
endfunction()

# soul_add_test_ui — UI 测试 (自动条件链接 Qt6::Widgets)
function(soul_add_test_ui TEST_NAME)
    cmake_parse_arguments(ARG "" "SUBDIR" "${SOUL_TEST_MULTI_VALUE_ARGS}" ${ARGN})

    if(ARG_SUBDIR)
        set(SRC "${ARG_SUBDIR}/${TEST_NAME}.cpp")
    else()
        # Auto-detect subdirectory
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit/${TEST_NAME}.cpp")
            set(SRC "unit/${TEST_NAME}.cpp")
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integration/${TEST_NAME}.cpp")
            set(SRC "integration/${TEST_NAME}.cpp")
        else()
            set(SRC "${TEST_NAME}.cpp")
        endif()
    endif()

    add_executable(${TEST_NAME} ${SRC})
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

    if(ARG_LABELS)
        set_tests_properties(${TEST_NAME} PROPERTIES LABELS "${ARG_LABELS}")
    endif()
endfunction()

# soul_add_test_widgets — Widgets 专属测试
function(soul_add_test_widgets TEST_NAME)
    cmake_parse_arguments(ARG "" "SUBDIR" "${SOUL_TEST_MULTI_VALUE_ARGS}" ${ARGN})

    if(TARGET Qt6::Widgets)
        if(ARG_SUBDIR)
            set(SRC "${ARG_SUBDIR}/${TEST_NAME}.cpp")
        else()
            # Auto-detect subdirectory
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit/${TEST_NAME}.cpp")
                set(SRC "unit/${TEST_NAME}.cpp")
            else()
                set(SRC "${TEST_NAME}.cpp")
            endif()
        endif()

        add_executable(${TEST_NAME} ${SRC})
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

        if(ARG_LABELS)
            set_tests_properties(${TEST_NAME} PROPERTIES LABELS "${ARG_LABELS}")
        endif()
    endif()
endfunction()

# soul_add_test_cxx20 — C++20 协程测试
function(soul_add_test_cxx20 TEST_NAME)
    cmake_parse_arguments(ARG "" "SUBDIR" "${SOUL_TEST_MULTI_VALUE_ARGS}" ${ARGN})

    if(ENABLE_CXX20)
        if(ARG_SUBDIR)
            set(SRC "${ARG_SUBDIR}/${TEST_NAME}.cpp")
        else()
            set(SRC "${TEST_NAME}.cpp")
        endif()

        add_executable(${TEST_NAME} ${SRC})
        target_link_libraries(${TEST_NAME} PRIVATE
            Qt6::Test
            Qt6::Core
            ${ARG_QT_LIBS}
            ${ARG_SOUL_LIBS}
        )
        windeployqt_post_build(${TEST_NAME} TRUE)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

        if(ARG_LABELS)
            set_tests_properties(${TEST_NAME} PROPERTIES LABELS "${ARG_LABELS}")
        endif()
    endif()
endfunction()

# ============================================================================
# Unit Tests — 单元测试 (单个类/函数的隔离测试)
# ============================================================================

# --- Core 层 ---
soul_add_test(test_result              SOUL_LIBS soul_core                              LABELS "unit;core")
soul_add_test(test_core                SOUL_LIBS soul_core                              LABELS "unit;core")
soul_add_test(test_feature_flags       SOUL_LIBS soul_core                              LABELS "unit;core")
soul_add_test(test_module_registry     SOUL_LIBS soul_core soul_logging soul_configuration  LABELS "unit;core")
soul_add_test(test_singleton           SOUL_LIBS soul_core                              LABELS "unit;core")
soul_add_test(test_json_feature_flag_provider SOUL_LIBS soul_core                       LABELS "unit;core")

# --- DI ---
soul_add_test(test_di                  SOUL_LIBS soul_di soul_core                      LABELS "unit;di")

# --- Logging ---
soul_add_test(test_logger              SOUL_LIBS soul_logging soul_core                  LABELS "unit;logging")

# --- Async ---
soul_add_test(test_async               SOUL_LIBS soul_async soul_core                   LABELS "unit;async")

# --- Event ---
soul_add_test(test_event_bus           SOUL_LIBS soul_event soul_core                   LABELS "unit;event")
soul_add_test(test_typed_event_bus     SOUL_LIBS soul_event soul_core soul_async soul_logging  LABELS "unit;event")

# --- Utils ---
soul_add_test(test_utils               SOUL_LIBS soul_utils soul_core   QT_LIBS Qt6::Network Qt6::Xml  LABELS "unit;utils")

# --- Configuration ---
soul_add_test(test_configuration       SOUL_LIBS soul_configuration soul_core            LABELS "unit;config")
soul_add_test(test_config_bind         SOUL_LIBS soul_configuration soul_core            LABELS "unit;config")
soul_add_test(test_config_center_client SOUL_LIBS soul_configuration soul_core           LABELS "unit;config")
soul_add_test(test_remote_config       SOUL_LIBS soul_configuration soul_core  QT_LIBS Qt6::Network  LABELS "unit;config")

# --- Storage ---
soul_add_test(test_storage             SOUL_LIBS soul_storage soul_core                  LABELS "unit;storage")

# --- Cache ---
soul_add_test(test_cache               SOUL_LIBS soul_storage soul_core                  LABELS "unit;cache")
soul_add_test(test_soul_cache          SOUL_LIBS soul_cache soul_core soul_logging       LABELS "unit;cache")
soul_add_test(test_redis_cache          SOUL_LIBS soul_cache soul_core                   LABELS "unit;cache;redis")  # v2.9.2

# v2.9.3: Service Discovery 测试
soul_add_test(test_discovery_lifecycle
    SOUL_LIBS soul_rpc soul_core  QT_LIBS Qt6::Network
    LABELS "integration;discovery;lifecycle")

soul_add_test(test_discovery_concurrency
    SOUL_LIBS soul_rpc soul_core  QT_LIBS Qt6::Network
    LABELS "concurrency;discovery")

# --- Validation ---
# test_validation.cpp not yet implemented (v3.0 deferred)

# --- ORM ---
soul_add_test(test_orm                 SOUL_LIBS soul_orm soul_database soul_data soul_core soul_logging  QT_LIBS Qt6::Sql  LABELS "unit;orm")
soul_add_test(test_query_wrapper       SOUL_LIBS soul_orm soul_core  QT_LIBS Qt6::Sql    LABELS "unit;orm")

# --- Observability ---
soul_add_test(test_observability       SOUL_LIBS soul_observability soul_logging soul_core  LABELS "unit;observability")
soul_add_test(test_otlp_exporter       SOUL_LIBS soul_observability soul_core soul_logging soul_async soul_network  QT_LIBS Qt6::Network  LABELS "unit;observability")

# --- AOP ---
soul_add_test(test_aop                 SOUL_LIBS soul_aop soul_core soul_logging         LABELS "unit;aop")

# --- Scheduler ---
soul_add_test(test_scheduler           SOUL_LIBS soul_scheduler soul_core soul_di        LABELS "unit;scheduler")

# --- Plugin ---
soul_add_test(test_plugin              SOUL_LIBS soul_plugin soul_di soul_core           LABELS "unit;plugin")

# ============================================================================
# Integration Tests — 集成测试 (多模块协作)
# ============================================================================

# --- Data + Database ---
soul_add_test(test_data                SOUL_LIBS soul_database soul_data soul_core  QT_LIBS Qt6::Sql  LABELS "integration;data")
soul_add_test(test_repository_factory  SOUL_LIBS soul_database soul_core soul_data soul_orm  QT_LIBS Qt6::Sql  LABELS "integration;data")

# --- Network ---
soul_add_test(test_http_request        SOUL_LIBS soul_network soul_core                   LABELS "integration;network")
soul_add_test(test_network             SOUL_LIBS soul_network soul_core soul_logging  QT_LIBS Qt6::Network  LABELS "integration;network")
soul_add_test(test_connection_manager  SOUL_LIBS soul_network soul_event soul_core soul_logging  QT_LIBS Qt6::Network  LABELS "integration;network")

# --- Auth ---
soul_add_test(test_auth                SOUL_LIBS soul_auth soul_network soul_storage soul_utils
                                                 soul_base soul_configuration soul_logging soul_core
                                        QT_LIBS Qt6::Network Qt6::Sql                    LABELS "integration;auth")
soul_add_test(test_oauth2              SOUL_LIBS soul_auth soul_base soul_core  QT_LIBS Qt6::Network  LABELS "integration;auth")

# --- MQ ---
soul_add_test(test_mq                  SOUL_LIBS soul_mq soul_core soul_logging soul_async  QT_LIBS Qt6::Network  LABELS "integration;mq")
soul_add_test(test_kafka_adapter       SOUL_LIBS soul_mq soul_core soul_logging soul_async  LABELS "integration;mq")

# --- RPC ---
soul_add_test(test_rpc                 SOUL_LIBS soul_rpc soul_core  QT_LIBS Qt6::Network  LABELS "integration;rpc")
soul_add_test(test_grpc_server         SOUL_LIBS soul_rpc soul_core  QT_LIBS Qt6::Network  LABELS "integration;rpc")
soul_add_test(test_service_discovery   SOUL_LIBS soul_rpc soul_core  QT_LIBS Qt6::Network  LABELS "integration;rpc")

# --- Server ---
soul_add_test(test_http_server         SOUL_LIBS soul_server soul_core soul_logging  QT_LIBS Qt6::Network  LABELS "integration;server")
soul_add_test(test_health              SOUL_LIBS soul_server soul_core soul_logging       LABELS "integration;server")
soul_add_test(test_websocket_server    SOUL_LIBS soul_server soul_core soul_logging  QT_LIBS Qt6::Network Qt6::WebSockets  LABELS "integration;server")

# --- Resource Pool Monitor (多模块协作) ---
soul_add_test(test_resource_pool_monitor SOUL_LIBS soul_observability soul_async soul_network
                                                  soul_database soul_data soul_core soul_logging
                                         QT_LIBS Qt6::Network Qt6::Sql                   LABELS "integration;observability")

# --- CS 架构集成 ---
soul_add_test(test_cs                  SOUL_LIBS soul_cs soul_core soul_di soul_ui soul_data soul_application  LABELS "integration;cs")
soul_add_test(test_cs_admin_panel      SOUL_LIBS soul_cs soul_core soul_di soul_ui soul_data soul_application  QT_LIBS Qt6::Widgets  LABELS "integration;cs")
soul_add_test(test_cs_ipc_router       SOUL_LIBS soul_cs soul_core soul_di               LABELS "integration;cs")

# --- Base (多模块依赖) ---
soul_add_test_ui(test_base             SOUL_LIBS soul_base soul_auth soul_network soul_storage
                                                 soul_utils soul_configuration soul_logging
                                                 soul_core soul_data soul_ui
                                        QT_LIBS Qt6::Network Qt6::Sql                    LABELS "integration;base")

# --- 版本综合测试 ---
soul_add_test(test_v193_components     SOUL_LIBS soul_server soul_network soul_validation
                                                 soul_observability soul_core soul_logging
                                        QT_LIBS Qt6::Network                              LABELS "integration")
soul_add_test(test_v194_components     SOUL_LIBS soul_server soul_network soul_async
                                                 soul_observability soul_scheduler soul_di
                                                 soul_core soul_logging
                                        QT_LIBS Qt6::Network                              LABELS "integration")

# ============================================================================
# Lifecycle Tests — 生命周期测试 [v2.6.0 新增]
# ============================================================================
# 验证 ILifecycle 语义: initialize→start→stop→shutdown 状态转换
# 验证失败回滚、逆序清理、幂等性

soul_add_test(test_lifecycle_basic     SUBDIR lifecycle
    SOUL_LIBS soul_core
    LABELS "lifecycle;core")

soul_add_test(test_lifecycle_rollback  SUBDIR lifecycle
    SOUL_LIBS soul_core
    LABELS "lifecycle;core")

soul_add_test(test_lifecycle_module    SUBDIR lifecycle
    SOUL_LIBS soul_core soul_logging
    LABELS "lifecycle;core")

soul_add_test(test_lifecycle_registry  SUBDIR lifecycle
    SOUL_LIBS soul_core soul_application soul_di
    LABELS "lifecycle;application")

# ============================================================================
# Concurrency Tests — 并发安全测试 [v2.6.0 新增]
# ============================================================================
# 验证线程安全: DI resolve、EventBus dispatch、Cache 读写、ConnectionPool acquire
# 推荐使用 TSan 运行: cmake -DENABLE_TSAN=ON && ctest -L concurrency

soul_add_test(test_concurrency_di      SUBDIR concurrency
    SOUL_LIBS soul_di soul_core
    LABELS "concurrency;di")

soul_add_test(test_concurrency_event   SUBDIR concurrency
    SOUL_LIBS soul_event soul_core soul_async soul_logging
    LABELS "concurrency;event")

soul_add_test(test_concurrency_cache   SUBDIR concurrency
    SOUL_LIBS soul_cache soul_core soul_logging
    LABELS "concurrency;cache")

soul_add_test(test_concurrency_result  SUBDIR concurrency
    SOUL_LIBS soul_core
    LABELS "concurrency;core")

# v2.9.0: Stress Tests
soul_add_test(test_stress_task        SUBDIR concurrency
    SOUL_LIBS soul_core
    LABELS "concurrency;stress;task")

soul_add_test(test_stress_connection  SUBDIR concurrency
    SOUL_LIBS soul_core
    LABELS "concurrency;stress;connection")

# ============================================================================
# Configuration Tests — 配置测试 [v2.9.0 新增]
# ============================================================================

soul_add_test(test_config_provider    SUBDIR configuration
    SOUL_LIBS soul_configuration soul_core
    LABELS "unit;configuration")

# ============================================================================
# Messaging Tests — 消息总线测试 [v2.9.1 新增]
# ============================================================================

soul_add_test(test_message_bus              SUBDIR messaging
    SOUL_LIBS soul_event soul_core
    LABELS "unit;messaging")

soul_add_test(test_message_bus_concurrency  SUBDIR messaging
    SOUL_LIBS soul_event soul_core
    LABELS "concurrency;messaging")

soul_add_test(test_message_bus_stress       SUBDIR messaging
    SOUL_LIBS soul_event soul_core
    LABELS "concurrency;stress;messaging")

# ============================================================================
# Contract Tests — 契约测试 [v2.6.0 新增]
# ============================================================================
# 验证四层架构依赖规则、API 兼容性

soul_add_test(test_contract_architecture SUBDIR contract
    SOUL_LIBS soul_core
    LABELS "contract;architecture")

soul_add_test(test_contract_error        SUBDIR contract
    SOUL_LIBS soul_core
    LABELS "contract;core")

# ============================================================================
# Application Layer — UI Tests
# ============================================================================

soul_add_test_ui(test_ui               SOUL_LIBS soul_ui soul_core soul_base soul_configuration  LABELS "unit;ui")
soul_add_test_ui(test_ui_switch        SOUL_LIBS soul_ui soul_core soul_base soul_configuration  LABELS "unit;ui")
soul_add_test_ui(test_ui_avatar        SOUL_LIBS soul_ui soul_core soul_base soul_configuration  LABELS "unit;ui")
soul_add_test_ui(test_ui_dropdown      SOUL_LIBS soul_ui soul_core soul_base soul_configuration  LABELS "unit;ui")
soul_add_test_ui(test_ui_window        SOUL_LIBS soul_ui soul_core soul_base soul_configuration  LABELS "unit;ui")

soul_add_test_widgets(test_ui_button       SOUL_LIBS soul_core soul_ui soul_logging  LABELS "unit;ui")
soul_add_test_widgets(test_ui_components1  SOUL_LIBS soul_core soul_ui soul_logging  LABELS "unit;ui")
soul_add_test_widgets(test_ui_components2  SOUL_LIBS soul_core soul_ui soul_logging  LABELS "unit;ui")
soul_add_test_widgets(test_ui_components3  SOUL_LIBS soul_core soul_ui soul_logging  LABELS "unit;ui")
soul_add_test_widgets(test_ui_components4  SOUL_LIBS soul_core soul_ui soul_logging  LABELS "unit;ui")
soul_add_test_widgets(test_ui_components5  SOUL_LIBS soul_core soul_ui soul_logging  LABELS "unit;ui")
soul_add_test_widgets(test_ui_components6  SOUL_LIBS soul_core soul_ui soul_logging  LABELS "unit;ui")

# ============================================================================
# Conditional Compilation
# ============================================================================

soul_add_test_cxx20(test_coroutine     SOUL_LIBS soul_async soul_core  LABELS "unit;async")
