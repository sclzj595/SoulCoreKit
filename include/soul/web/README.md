# soul::web 模块 — QtWebEngine 集成 (预留)

> **状态**: 预留，未实现
> **目标版本**: 待 QtWebEngine 评估后决定
> **设计原则**: 不提前架构污染，仅预留扩展点

---

## 启用方式 (未来)

1. CMake: `-DSOUL_WEB_ENABLED=ON`
2. 配置: `soul.web.enabled: true`

## 架构设计 (仅供参考)

```
CsController (Signal/Slot)  ←→  Web Adapter (JS Bridge)
       ↓                              ↓
CsRouter (页面导航)           ←→  WebRouter (URL 路由)
       ↓                              ↓
QWidget (Qt 原生)              ←→  QWebEngineView (Web 渲染)
```

## 核心原则

**业务 Service 不应该知道自己是 CS 还是 Web。**

```
                  UserService
                      ▲
            ┌─────────┴─────────┐
       CsController        Web Adapter
```

Web Adapter 的职责仅限于：
- 将 HTTP 请求转换为 CsController 的 Signal 调用
- 复用已有的 CsService 和 CsRouter
- **不复制业务逻辑**

## 依赖

- Qt 6.5+ WebEngine (约 80MB 额外体积)
- 建议仅在需要 Web UI 的场景启用

## 相关文档

- [v2.5.0-cs-architecture.md](../docs/v2.5.0/v2.5.0-cs-architecture.md) — 第七章：CS 与 Web 共享业务架构