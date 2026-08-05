# 09 — 技术栈与依赖

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 9.1 核心依赖

| 依赖 | 版本 | 用途 | 类型 |
|------|------|------|------|
| **Qt 6.5.3 LTS** | Core, Network, WebSockets, Sql, Xml, Widgets, Gui | 核心框架 | 必需 |
| **nlohmann/json** | 3.11.3 | JSON 解析 (非 UI 模块) | 必需 |
| **spdlog** | 1.14.1 | 高性能结构化日志 | 必需 |
| **amqpcpp** | 4.3.27 | RabbitMQ 真实后端 | 可选 |
| **ZLIB** | 系统 | 压缩工具 | 条件必需 |

---

## 9.2 nlohmann/json 三级备选策略

```
1. find_package(nlohmann_json)  ← CI 环境 (apt install nlohmann-json3-dev)
2. 本地路径 G:/MinGW/nlohmann   ← 开发者本地 Windows
3. FetchContent Git shallow clone ← 最终备选 (GitHub v3.11.3)
```

---

## 9.3 spdlog 三级备选策略

```
1. find_package(spdlog)          ← CI 环境 (apt install libspdlog-dev)
2. 本地路径 G:/MinGW/spdlog      ← 开发者本地 Windows
3. FetchContent Git shallow clone ← 最终备选 (GitHub v1.14.1)
```

---

## 9.4 编译器支持

| 编译器 | 最低版本 | 平台 |
|--------|----------|------|
| GCC | 11 | Linux |
| MSVC | 2019 (16.0) | Windows |
| Clang | 14 | macOS |
| MinGW | 11.2.0 (Qt-bundled) | Windows |

---

## 9.5 CMake 配置

| 配置项 | 值 |
|--------|-----|
| CMake 最低版本 | 3.16 |
| C++ 标准 | C++17 |
| C++ 标准要求 | REQUIRED |
| AUTOMOC | ON |
| 默认构建类型 | 静态库 |

---

## 9.6 平台特定配置

### Windows
- `NOMINMAX` 定义 (防止 windows.h min/max 宏冲突)
- `windeployqt` 自动部署 Qt DLL
- `qoffscreen.dll` 平台插件自动复制

### Linux
- ZLIB 系统依赖 (REQUIRED)
- `-Wl,--gc-sections` 链接优化

### macOS
- ZLIB 通过 Qt 提供 (QUIET)

---

## 9.7 编译优化

| 构建类型 | 优化选项 |
|----------|----------|
| **Release** | MSVC: `/O2 /Ob2 /Oi /Ot /Oy /GL /LTCG` |
|  | GCC/Clang: `-O3 -march=native -mtune=native` |
| **Debug** | 无优化，保留调试符号 |
| **TSan** | `-fsanitize=thread -g -O1 -fno-omit-frame-pointer` |
| **ASAN/UBSAN** | `-fsanitize=address,undefined` |
| **Coverage** | `--coverage -O0 -g` |