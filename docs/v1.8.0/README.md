# SoulCoreKit v1.8.0 迭代规划

**文档状态**: Accepted
**创建日期**: 2026-07-26
**当前基线**: v1.7.0 (已发布,2026-07-26)
**目标版本**: v1.8.0
**版本类型**: Minor(功能新增,向后兼容)
**发布策略**: 按范围发布(完成 P0+P1 后发布,无时间盒)

---

## 1. 主题

**CI 质量闭环 + 协议增强 + v1.7.0 延期项收敛**

v1.8.0 聚焦三件事:
1. **CI 质量闭环**:TSan 接入 Linux CI,补齐 v1.7.0 P0-C 延期项
2. **协议增强**:HTTP/2 多路复用支持,提升 RPC 与 HttpClient 的并发性能
3. **恰当补强**:Clang-Tidy CI、RPC 测试深度等作为 P2 可选,允许跨版本延期

---

## 2. 范围

| 优先级 | 任务 | 来源 |
|--------|------|------|
| P0 必做 | TSan CI 接入 | v1.7.0 P0-C 延期 |
| P1 必做 | HTTP/2 多路复用 + HttpClient 连接池 | v1.7.0 P2 延期 + project_memory 约束 |
| P2 可选 | Clang-Tidy CI / RPC 测试深度 / AmqpCpp 集成测试 / 配置环境隔离 | v1.7.0 延期小项 |

### 2.1 已确认决策

| 决策项 | 结论 |
|--------|------|
| P0 范围 | TSan CI 接入 |
| P1 范围 | HTTP/2 多路复用 + HttpClient 连接池 |
| L3 Redis | 继续暂缓 |
| OpenTelemetry | 继续暂缓 |
| OAuth2/OIDC | 暂缓到 v1.9.0 |
| SoulGateway | 暂缓到 v1.9.0 |
| 发布策略 | 按范围发布 |

### 2.2 排除项

- L3 Redis / Memcached 分布式缓存
- OpenTelemetry / OTLP exporter
- OAuth2/OIDC 认证流程
- SoulGateway API 网关
- 备份与恢复机制
- C++20 Coroutines

---

## 3. 详细设计文档

| 文档 | 内容 | 优先级 |
|------|------|--------|
| [01_tsan_ci_design.md](./01_tsan_ci_design.md) | TSan CI 接入设计 | P0 |
| [02_http2_design.md](./02_http2_design.md) | HTTP/2 多路复用设计 | P1 |
| [03_http_client_pool_design.md](./03_http_client_pool_design.md) | HttpClient 连接池设计 | P1 |

---

## 4. 里程碑

| 里程碑 | 验收标准 |
|--------|----------|
| M0 — P0 CI 闭环 | TSan workflow 绿色;多线程测试套件零警告 |
| M1 — P1 协议增强 | HTTP/2 启用且向后兼容;连接池配置可用;新增测试通过 |
| M2 — v1.8.0 发布 | 全量测试通过;CHANGELOG 更新;版本号同步 |

---

## 5. 版本号同步清单

发布时需同步以下文件到 `1.8.0`:
- `CMakeLists.txt` (project VERSION)
- `Doxyfile` (PROJECT_NUMBER)
- `CITATION.cff` (version)
- `README.md` (版本徽章)
- `CHANGELOG.md` (新增 `## [1.8.0]` 章节)
