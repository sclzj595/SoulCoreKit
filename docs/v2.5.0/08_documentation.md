# 08 — 文档体系

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 8.1 规范文档 (15+ 篇)

| 编号 | 文档 | 路径 | 语言 |
|------|------|------|------|
| 00 | 项目愿景 | `docs/00_vision.md` | EN / ZH |
| 01 | 架构规范 | `docs/01_architecture.md` | EN / ZH |
| 02 | 设计原则 | `docs/02_design_principles.md` | EN / ZH |
| 03 | 模块规范 | `docs/03_module_specification.md` | EN / ZH |
| 04 | UI 规范 | `docs/04_ui_specification.md` | EN / ZH |
| 05 | 编码风格 | `docs/05_coding_style.md` | EN / ZH |
| 06 | API 设计 | `docs/06_api_design.md` | EN / ZH |
| 07 | 线程安全 | `docs/07_threading.md` | EN / ZH |
| 08 | 错误处理 | `docs/08_error_handling.md` | EN / ZH |
| 09 | 内存管理 | `docs/09_memory_management.md` | EN / ZH |
| 10 | 构建系统 | `docs/10_build_system.md` | EN / ZH |
| 11 | 测试规范 | `docs/11_testing.md` | EN / ZH |
| 12 | 版本管理 | `docs/12_versioning.md` | EN / ZH |
| 13 | 贡献指南 | `docs/13_contributing.md` | EN / ZH |
| 14 | 路线图 | `docs/14_roadmap.md` | EN / ZH |

---

## 8.2 架构决策记录 (ADR)

| 编号 | 决策 | 路径 |
|------|------|------|
| ADR-001 | 错误处理边界规则 | `docs/adr/001-error-handling-boundary.md` |
| ADR-002 | 模块依赖规则 | `docs/adr/002-module-dependency-rules.md` |
| ADR-003 | 内存管理策略 | `docs/adr/003-memory-management.md` |
| ADR-004 | ORM 多数据库架构 | `docs/adr/004-or-multi-database.md` |
| ADR-005 | 线程安全策略 | `docs/adr/005-thread-safety-policy.md` |

---

## 8.3 版本设计文档

| 版本 | 路径 | 内容 |
|------|------|------|
| v1.7.0 | `docs/v1.7.0/` | 缓存、可观测性、ORM 增强、MQ 集成 |
| v1.8.0 | `docs/v1.8.0/` | TSan CI、HTTP/2、连接池 |
| v1.9.4 | `docs/v1.9.4/` | 发布说明 |
| v2.0.0 | `docs/v2.0.0/` | Application 启动器、YAML 配置、ORM 增强 |
| v2.1.0 | `docs/v2.1.0/` | CS 架构定义 |
| v2.5.0 | `docs/v2.5.0/` | 三层架构定稿、项目分析文档 |

---

## 8.4 其他文档

| 文档 | 路径 | 说明 |
|------|------|------|
| README | `README.md` | 项目主文档 |
| CHANGELOG | `CHANGELOG.md` | 版本变更日志 |
| 框架介绍 | `docs/SoulCoreKit_Framework_Introduction.md` | 框架介绍 |
| 技术债审计 | `docs/tech_debt_audit.md` | 技术债审计报告 |
| 脚手架差距 | `docs/scaffold_gap_roadmap.md` | 脚手架缺失功能路线图 |
| 产品需求 | `docx/` | 历史 PRD 文档 |

---

## 8.5 文档风格

- **RFC 风格**: 工业级框架规范，非产品需求文档
- **中英文双版**: 核心规范文档均有中英文版本
- **版本绑定**: 设计文档与版本号绑定，可追溯
- **ADR 驱动**: 重大架构决策通过 ADR 记录