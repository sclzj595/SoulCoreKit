# ============================================================================
# SoulCoreKitArchitecture.cmake — 四层架构依赖规则强制检查 [v2.6.0]
# ============================================================================
# 架构分层:
#   Layer 0: Core           — CS/BS 共用极稳定核心 (零业务绑定)
#   Layer 1: Infrastructure — CS/BS 共用基础设施
#   Layer 2: Extensions     — 企业级扩展能力 (可选)
#   Layer 3: Application    — 业务场景适配 (CS/BS 各自独立)
#
# 依赖方向 (严格单向):
#   Application → Extensions → Infrastructure → Core
#
# 允许:
#   Application → Extensions / Infrastructure / Core
#   Extensions  → Infrastructure / Core
#   Infrastructure → Core
#
# 禁止:
#   Core → Infrastructure / Extensions / Application
#   Infrastructure → Extensions / Application
#   Extensions → Application
# ============================================================================

# --- 模块分层定义 ---

set(SC_LAYER_CORE
    soul_core
    soul_di
    soul_logging
    soul_async
    soul_event
)

set(SC_LAYER_INFRASTRUCTURE
    soul_network_core
    soul_network_policy
    soul_network_http
    soul_network_protocol
    soul_network
    soul_data
    soul_database
    soul_orm
    soul_storage
    soul_cache
    soul_base
    soul_utils
    soul_configuration
    soul_observability
    soul_validation
)

set(SC_LAYER_EXTENSIONS
    soul_rpc
    soul_mq
    soul_auth
    soul_plugin
    soul_scheduler
    soul_server
    soul_aop
)

set(SC_LAYER_APPLICATION
    soul_application
    soul_ui
    soul_cs
)

# --- 依赖约束矩阵 ---
# 格式: target → allowed_layers
# 仅允许 target 依赖 declared_layers 中的模块

function(sc_get_layer TARGET OUT_LAYER)
    if("${TARGET}" IN_LIST SC_LAYER_CORE)
        set(${OUT_LAYER} "Core" PARENT_SCOPE)
    elseif("${TARGET}" IN_LIST SC_LAYER_INFRASTRUCTURE)
        set(${OUT_LAYER} "Infrastructure" PARENT_SCOPE)
    elseif("${TARGET}" IN_LIST SC_LAYER_EXTENSIONS)
        set(${OUT_LAYER} "Extensions" PARENT_SCOPE)
    elseif("${TARGET}" IN_LIST SC_LAYER_APPLICATION)
        set(${OUT_LAYER} "Application" PARENT_SCOPE)
    else()
        set(${OUT_LAYER} "Unknown" PARENT_SCOPE)
    endif()
endfunction()

# 返回指定 layer 允许依赖的所有模块列表
function(sc_get_allowed_modules LAYER OUT_MODULES)
    set(MODULES "")
    if(LAYER STREQUAL "Core")
        # Core 层不允许依赖任何 SoulCoreKit 模块 (仅允许 Qt + 第三方)
        # 空列表 = 不允许依赖任何内部模块
    elseif(LAYER STREQUAL "Infrastructure")
        set(MODULES ${SC_LAYER_CORE})
    elseif(LAYER STREQUAL "Extensions")
        set(MODULES ${SC_LAYER_CORE} ${SC_LAYER_INFRASTRUCTURE})
    elseif(LAYER STREQUAL "Application")
        set(MODULES ${SC_LAYER_CORE} ${SC_LAYER_INFRASTRUCTURE} ${SC_LAYER_EXTENSIONS})
    endif()
    set(${OUT_MODULES} "${MODULES}" PARENT_SCOPE)
endfunction()

# --- 核心检查函数 ---
# sc_check_architecture()
# 遍历所有 SoulCoreKit 模块的 LINK_LIBRARIES，验证依赖方向

function(sc_check_architecture)
    message(STATUS "SoulCoreKit Architecture: checking layer dependency rules...")

    set(ALL_MODULES
        ${SC_LAYER_CORE}
        ${SC_LAYER_INFRASTRUCTURE}
        ${SC_LAYER_EXTENSIONS}
        ${SC_LAYER_APPLICATION}
    )

    set(VIOLATIONS_FOUND FALSE)

    foreach(MODULE IN LISTS ALL_MODULES)
        if(NOT TARGET ${MODULE})
            continue()
        endif()

        sc_get_layer(${MODULE} MODULE_LAYER)
        sc_get_allowed_modules(${MODULE_LAYER} ALLOWED_MODULES)

        # 获取模块的直接链接依赖
        get_target_property(LINK_LIBS ${MODULE} LINK_LIBRARIES)
        if(NOT LINK_LIBS)
            set(LINK_LIBS "")
        endif()

        # 也检查 INTERFACE_LINK_LIBRARIES
        get_target_property(IFACE_LINK_LIBS ${MODULE} INTERFACE_LINK_LIBRARIES)
        if(NOT IFACE_LINK_LIBS)
            set(IFACE_LINK_LIBS "")
        endif()

        set(ALL_LINK_LIBS ${LINK_LIBS} ${IFACE_LINK_LIBS})

        foreach(DEP IN LISTS ALL_LINK_LIBS)
            # 只检查 SoulCoreKit 内部模块
            if(NOT ("${DEP}" IN_LIST ALL_MODULES))
                continue()
            endif()

            sc_get_layer(${DEP} DEP_LAYER)

            # 同层内部依赖: 始终允许 (如 soul_di→soul_core, soul_observability→soul_data)
            if(MODULE_LAYER STREQUAL DEP_LAYER)
                continue()
            endif()

            # 下层依赖: 允许 (Application→Extensions/Infra/Core, Extensions→Infra/Core, Infrastructure→Core)
            # Core 层没有下层, 所以 Core→anything 已经在上面的同层检查中排除
            if(MODULE_LAYER STREQUAL "Application")
                # Application 可以依赖任何下层 → 允许
                continue()
            elseif(MODULE_LAYER STREQUAL "Extensions" AND
                   ("${DEP}" IN_LIST SC_LAYER_CORE OR "${DEP}" IN_LIST SC_LAYER_INFRASTRUCTURE))
                continue()
            elseif(MODULE_LAYER STREQUAL "Infrastructure" AND ("${DEP}" IN_LIST SC_LAYER_CORE))
                continue()
            endif()

            # 反向依赖: 违规 (Core→Infra/Ext/App, Infra→Ext/App, Ext→App)
            message(SEND_ERROR
                "ARCHITECTURE VIOLATION: ${MODULE} (${MODULE_LAYER} layer) depends on ${DEP} (${DEP_LAYER} layer). "
                "Allowed dependency direction: Application → Extensions → Infrastructure → Core."
            )
            set(VIOLATIONS_FOUND TRUE)
        endforeach()
    endforeach()

    if(VIOLATIONS_FOUND)
        message(FATAL_ERROR
            "SoulCoreKit architecture check FAILED. "
            "Fix the above violations before building. "
            "See cmake/SoulCoreKitArchitecture.cmake for layer dependency rules."
        )
    else()
        message(STATUS "SoulCoreKit Architecture: all layer dependencies verified OK")
    endif()
endfunction()

# --- 便捷函数: 检查单个模块是否属于指定层 ---
function(sc_assert_layer TARGET EXPECTED_LAYER)
    sc_get_layer(${TARGET} ACTUAL_LAYER)
    if(NOT ACTUAL_LAYER STREQUAL EXPECTED_LAYER)
        message(FATAL_ERROR
            "ARCHITECTURE ERROR: ${TARGET} is in ${ACTUAL_LAYER} layer, expected ${EXPECTED_LAYER}"
        )
    endif()
endfunction()

# --- 打印架构概览 (调试用) ---
function(sc_print_architecture)
    message(STATUS "=== SoulCoreKit Architecture Overview ===")
    message(STATUS "Core (${SC_LAYER_CORE_COUNT} modules):")
    foreach(M IN LISTS SC_LAYER_CORE)
        message(STATUS "  - ${M}")
    endforeach()
    message(STATUS "Infrastructure (${SC_LAYER_INFRASTRUCTURE_COUNT} modules):")
    foreach(M IN LISTS SC_LAYER_INFRASTRUCTURE)
        message(STATUS "  - ${M}")
    endforeach()
    message(STATUS "Extensions:")
    foreach(M IN LISTS SC_LAYER_EXTENSIONS)
        message(STATUS "  - ${M}")
    endforeach()
    message(STATUS "Application:")
    foreach(M IN LISTS SC_LAYER_APPLICATION)
        message(STATUS "  - ${M}")
    endforeach()
endfunction()
