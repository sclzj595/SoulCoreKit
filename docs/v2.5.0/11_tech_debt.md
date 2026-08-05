# 11 — 技术债务

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 11.1 已知技术债

### TD-001: soul_application ↔ soul_cs 循环依赖

**严重程度**: P1 (Major)

**现状**: `soul_application` 定义为 INTERFACE 库（仅头文件），其实现文件（`service_registry.cpp`、`controller_registry.cpp`、`application_context.cpp`）归入 `soul_cs` 编译。`service_registry.h` 包含 `soul/cs/cs_service.h`，`controller_registry.h` 包含 `soul/cs/cs_controller.h`，形成 `application/` 层依赖 `cs/` 层的反向依赖。

**影响**:
- `soul_application` 不能独立使用，必须与 `soul_cs` 同时链接
- 无法单独测试 Application 层组件（必须编译整个 `soul_cs`）

**解决方向** (v3.0):
- 提取 `ILifecycleManaged` 接口到 `soul/core/`，解耦 ServiceRegistry 对 CsService 的依赖
- 提取 `IRouteHandler` 接口到 `soul/application/`，解耦 ControllerRegistry 对 CsController 的依赖
- 将 `soul_application` 从 INTERFACE 改为 STATIC 库，实现文件独立编译

**当前状态**: 已通过 `ILifecycleManaged` 部分解耦，但 `soul_application` 实现仍编译到 `soul_cs`。

---

### TD-002: CsController 继承 QWidget（通过 sc::Page）

**严重程度**: P2 (Minor)

**现状**: `CsController` 继承 `sc::Page` → `sc::ui::BaseWidget` → `QWidget`。Controller 同时承担路由分发、页面生命周期和 UI 渲染职责。

**理由**: 在桌面 CS 架构中，路由分发和页面生命周期是天然耦合的——导航到路径意味着创建 Controller 实例并推入页面栈。这是有意简化，而非架构疏忽。

**解决方向** (v3.0):
- 提取 `ICsRouteHandler` 纯虚接口，分离路由分发与 QWidget 生命周期
- Controller 实现 `ICsRouteHandler`，通过组合（而非继承）持有 Page 实例

**当前状态**: 当前设计有意为之，非紧急修复项。

---

### TD-003: cs/ 扁平目录结构

**严重程度**: P3 (Low)

**现状**: `cs/` 目录下 14 个头文件扁平排列，未按职责分子目录。

**解决方向** (v3.0):
- 重组为 `cs/router/`、`cs/controller/`、`cs/service/`、`cs/viewmodel/` 等子目录
- 当前阶段避免大规模文件移动以保持稳定性

---

## 11.2 技术债统计

| 编号 | 严重程度 | 状态 | 目标版本 |
|------|----------|------|----------|
| TD-001 | P1 Major | 已部分修复 | v3.0 |
| TD-002 | P2 Minor | 有意为之 | v3.0 |
| TD-003 | P3 Low | 延后 | v3.0 |

---

## 11.3 技术债处理原则

1. **不阻塞发布**: 已知技术债不阻塞当前版本发布
2. **版本绑定**: 每项技术债绑定目标修复版本
3. **渐进修复**: 在后续迭代中逐步解决，不追求一次性修复
4. **ADR 记录**: 重大架构决策通过 ADR 记录原因和后果