# TSan CI 接入设计(v1.8.0 P0-A)

**文档状态**: Accepted
**优先级**: P0(必做)
**来源**: v1.7.0 P0-C 延期项,ADR-005 §4 Enforcement 承诺

---

## 1. 现状分析

### 1.1 已有资产
- `.github/workflows/ci.yml`:Ubuntu/Windows 双平台构建+测试
- `cmake/tsan_suppressions.txt`:v1.7.0 已预创建,覆盖 Qt/Singleton/标准库/测试辅助 4 类误报
- `CMakeLists.txt`:已有 `ENABLE_WARNINGS`/`ENABLE_LTO`/`ENABLE_COVERAGE` 等 option 模式

### 1.2 缺失
- Linux 节点未启用 TSan 编译选项
- 无 TSan 专用 CI workflow
- 未在多线程测试套件上运行 TSan 检测

---

## 2. 实现方案

### 2.1 CMake option

在 `CMakeLists.txt` 添加:
```cmake
option(ENABLE_TSAN "Enable ThreadSanitizer (Linux only)" OFF)

if(ENABLE_TSAN)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR NOT UNIX)
        message(FATAL_ERROR "ENABLE_TSAN only supported on Linux with GCC")
    endif()
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
endif()
```

### 2.2 新增 workflow

`.github/workflows/tsan.yml`:仅在 Linux 节点运行,push/PR 时触发。

关键步骤:
1. Checkout
2. 安装 Qt 6.5.3 + g++
3. CMake 配置:`-DENABLE_TSAN=ON`
4. 构建
5. 运行 ctest,设置 `TSAN_OPTIONS`

### 2.3 TSan 运行环境

```
TSAN_OPTIONS="suppressions=${{ github.workspace }}/cmake/tsan_suppressions.txt \
              halt_on_error=0 \
              second_deadlock_stack=1 \
              report_bugs=1 \
              report_thread_leaks=0"
```

### 2.4 目标测试套件

多线程相关测试:
- `test_async`(Future/TaskRunner/ThreadPool)
- `test_event_bus`/`test_typed_event_bus`(事件分发)
- `test_cache`(MemoryCache/DiskCache/MultiLevelCache 锁)
- `test_mq`(InMemoryAmqpBackend dispatchLoop)
- `test_di`(DI Container 并发 resolve)

---

## 3. 验收标准

- TSan workflow 绿色通过
- 多线程测试套件零警告(明确误报通过 suppression 注解)
- 每条 suppression 必须有注释说明误报原因
- `cmake/tsan_suppressions.txt` 中无 SoulCoreKit 真实 bug 被掩盖

---

## 4. 风险与缓解

| 风险 | 缓解 |
|------|------|
| TSan 暴露大量潜在竞争 | 仅标记明确竞争,误报通过 suppression;非 v1.8.0 引入的竞争允许标记为 tech_debt 延期 |
| Linux CI 节点工具链差异 | 固定 Ubuntu 22.04 + GCC 11+ |
| suppression 文件失控 | 禁止宽泛模式(如 `race:std::shared_ptr*`),必须精确到函数 |
