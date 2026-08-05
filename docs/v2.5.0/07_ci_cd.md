# 07 — CI/CD 体系

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 7.1 工作流矩阵

| 工作流 | 文件 | 触发条件 | 平台 |
|--------|------|----------|------|
| Build | `build.yml` | push / PR | Windows, Linux, macOS |
| CI | `ci.yml` | push / PR | Windows, Linux |
| Lint | `lint.yml` | push / PR | Linux |
| Release | `release.yml` | 版本 tag | Windows, Linux, macOS |
| TSan | `tsan.yml` | push / PR | Linux |

---

## 7.2 各工作流详情

### Build (`build.yml`)
- 多平台构建验证
- 确认所有平台编译通过
- 基础冒烟测试

### CI (`ci.yml`)
- 完整 CI 流程
- 构建 + 测试 + 覆盖率报告
- 代码覆盖率 (lcov + genhtml + gcovr)

### Lint (`lint.yml`)
- clang-format 格式检查
- clang-tidy 静态分析
- CppCheck 代码质量检查

### Release (`release.yml`)
- 版本 tag 触发
- 多平台打包 (ZIP / DEB / RPM / NSIS / DragNDrop)
- 自动发布到 GitHub Releases

### TSan (`tsan.yml`)
- ThreadSanitizer 并发安全检测
- Ubuntu 22.04 + GCC
- `-fsanitize=thread -fno-omit-frame-pointer -g`
- 使用 `cmake/tsan_suppressions.txt` 抑制已知 Qt 内部数据竞争

---

## 7.3 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTS` | ON | 构建测试套件 |
| `BUILD_EXAMPLES` | ON | 构建示例 |
| `BUILD_DOCS` | OFF | 构建 Doxygen 文档 |
| `BUILD_SHARED_LIBS` | OFF | 构建动态库 (默认静态库) |
| `ENABLE_WARNINGS` | ON | 编译警告 (-Wall -Wextra -Werror) |
| `ENABLE_SANITIZERS` | OFF | ASAN/UBSAN 地址/未定义行为检测 |
| `ENABLE_TSAN` | OFF | ThreadSanitizer 线程安全检测 |
| `ENABLE_LTO` | OFF | 链接时优化 |
| `ENABLE_COVERAGE` | OFF | 代码覆盖率 |
| `ENABLE_CXX20` | OFF | C++20 协程支持 |
| `ENABLE_RABBITMQ` | OFF | RabbitMQ 真实后端 (仅 Linux/macOS) |

---

## 7.4 质量门禁

| 检查项 | 工具 | 阈值 |
|--------|------|------|
| 编译 | CMake + GCC/MSVC/Clang | 0 错误 |
| 单元测试 | Qt Test + CTest | 100% 通过 |
| 格式化 | clang-format | 无违规 |
| 静态分析 | clang-tidy | 无 Critical/Major |
| 并发安全 | TSan | 0 数据竞争 |
| 代码覆盖率 | lcov/gcovr | 目标 >90% (核心模块) |

---

## 7.5 覆盖率报告

- **HTML**: `cmake --build build --target coverage` (lcov + genhtml)
- **XML**: `cmake --build build --target coverage-xml` (gcovr Cobertura)
- **JSON**: `cmake --build build --target coverage-json` (gcovr JSON)
- **HTML (gcovr)**: `cmake --build build --target coverage-html`