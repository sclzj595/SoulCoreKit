# 06 — 测试体系

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 6.1 测试统计

| 层级 | 测试文件数 | 测试类型 |
|------|-----------|----------|
| Foundation 层 | 30+ | 单元测试 |
| Application 层 | 11 | UI 测试 (Widgets 条件) |
| 版本综合测试 | 2 | 集成测试 |
| 条件编译测试 | 1 | C++20 协程测试 |
| **总计** | **~50** | |

---

## 6.2 测试文件清单

### Foundation 层

| 测试文件 | 被测模块 |
|----------|----------|
| `test_result.cpp` | soul_core (Result<T>) |
| `test_core.cpp` | soul_core |
| `test_module_registry.cpp` | soul_core, soul_logging |
| `test_di.cpp` | soul_di, soul_core |
| `test_logger.cpp` | soul_logging, soul_core |
| `test_data.cpp` | soul_database, soul_data, soul_core |
| `test_repository_factory.cpp` | soul_database, soul_core, soul_data, soul_orm |
| `test_base.cpp` | soul_base, soul_auth, soul_network, soul_storage, soul_utils, soul_configuration, soul_logging, soul_core, soul_data, soul_ui |
| `test_utils.cpp` | soul_utils, soul_core |
| `test_configuration.cpp` | soul_configuration, soul_core |
| `test_config_bind.cpp` | soul_configuration, soul_core |
| `test_remote_config.cpp` | soul_configuration, soul_core |
| `test_storage.cpp` | soul_storage, soul_core |
| `test_cache.cpp` | soul_storage, soul_core |
| `test_soul_cache.cpp` | soul_cache, soul_core, soul_logging |
| `test_async.cpp` | soul_async, soul_core |
| `test_event_bus.cpp` | soul_event, soul_core |
| `test_typed_event_bus.cpp` | soul_event, soul_core, soul_async, soul_logging |
| `test_http_request.cpp` | soul_network, soul_core |
| `test_network.cpp` | soul_network, soul_core, soul_logging |
| `test_connection_manager.cpp` | soul_network, soul_event, soul_core, soul_logging |
| `test_auth.cpp` | soul_auth, soul_network, soul_storage, soul_utils, soul_base, soul_configuration, soul_logging, soul_core |
| `test_oauth2.cpp` | soul_auth, soul_base, soul_core |
| `test_mq.cpp` | soul_mq, soul_core, soul_logging, soul_async |
| `test_orm.cpp` | soul_orm, soul_database, soul_data, soul_core, soul_logging |
| `test_observability.cpp` | soul_observability, soul_logging, soul_core |
| `test_resource_pool_monitor.cpp` | soul_observability, soul_async, soul_network, soul_database, soul_data, soul_core, soul_logging |
| `test_aop.cpp` | soul_aop, soul_core, soul_logging |
| `test_scheduler.cpp` | soul_scheduler, soul_core, soul_di |
| `test_http_server.cpp` | soul_server, soul_core, soul_logging |
| `test_health.cpp` | soul_server, soul_core, soul_logging |
| `test_websocket_server.cpp` | soul_server, soul_core, soul_logging |
| `test_rpc.cpp` | soul_rpc, soul_core |
| `test_plugin.cpp` | soul_plugin, soul_di, soul_core |

### Application 层

| 测试文件 | 被测模块 |
|----------|----------|
| `test_ui.cpp` | soul_ui, soul_core, soul_base, soul_configuration |
| `test_ui_switch.cpp` | soul_ui, soul_core, soul_base, soul_configuration |
| `test_ui_avatar.cpp` | soul_ui, soul_core, soul_base, soul_configuration |
| `test_ui_dropdown.cpp` | soul_ui, soul_core, soul_base, soul_configuration |
| `test_ui_window.cpp` | soul_ui, soul_core, soul_base, soul_configuration |
| `test_ui_button.cpp` | soul_core, soul_ui, soul_logging |
| `test_ui_components1.cpp` | soul_core, soul_ui, soul_logging (GlassWidget/Animation/IconManager) |
| `test_ui_components2.cpp` | soul_core, soul_ui, soul_logging (BaseDialog/BaseViewModel) |
| `test_ui_components3.cpp` | soul_core, soul_ui, soul_logging (Checkbox/Card/Badge/Input/Progress) |
| `test_ui_components4.cpp` | soul_core, soul_ui, soul_logging (Slider/Spinner/TabBar/Toast/ToolTip) |
| `test_ui_components5.cpp` | soul_core, soul_ui, soul_logging (Loading/EmptyWidget/Page/SideBar/Navigation) |
| `test_ui_components6.cpp` | soul_core, soul_ui, soul_logging (BaseWidget/BaseView/Icon/ScrollBar) |

### 版本综合测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `test_v193_components.cpp` | CircuitBreaker / RateLimiter / Validator / InfoEndpoint / LoggersEndpoint |
| `test_v194_components.cpp` | Metrics/ThreadDump/Beans/Caches/ScheduledTasks/Shutdown + ThreadPool PriorityTask + ConnectionPool 动态扩缩容 + OtlpExporter |

### 条件编译测试

| 测试文件 | 条件 |
|----------|------|
| `test_coroutine.cpp` | C++20 (ENABLE_CXX20=ON) |

---

## 6.3 测试辅助函数

CMake 中定义了 4 个测试辅助函数，消除样板代码：

| 函数 | 用途 |
|------|------|
| `soul_add_test()` | 标准测试 (无 UI 依赖) |
| `soul_add_test_ui()` | UI 测试 (条件链接 Widgets) |
| `soul_add_test_widgets()` | Widgets 专属测试 |
| `soul_add_test_cxx20()` | C++20 协程测试 |

---

## 6.4 测试覆盖原则

- 所有 27 个模块均有对应测试
- 覆盖核心功能、边界条件、并发安全 (TSan)
- 新功能开发必须同步编写测试
- CI 流水线自动运行全量测试