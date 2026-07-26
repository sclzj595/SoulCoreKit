# 01 — v1.7.0 稳定化工作

**优先级**: P0(必做)
**目标**: 闭环 v1.6.x 遗留问题,确保 v1.7.0 在工业级稳定基线上启动

---

## 1. TSan 接入 CI

### 1.1 背景

ADR-005 §4 承诺接入 ThreadSanitizer,但 v1.6.x 未完成。TSan 可在运行时检测数据竞争,是防止并发回归的关键工具。

### 1.2 实现方案

**CI 配置**(`.github/workflows/tsan.yml` 新增):

```yaml
name: ThreadSanitizer
on: [push, pull_request]
jobs:
  tsan:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install Qt
        run: sudo apt-get install -y qt6-base-dev
      - name: Configure with TSan
        run: |
          cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
            -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
            -DBUILD_TESTS=ON
      - name: Build
        run: cmake --build build-tsan -j$(nproc)
      - name: Run tests under TSan
        run: |
          cd build-tsan
          ctest --output-on-failure
        env:
          TSAN_OPTIONS: "halt_on_error=1 second_deadlock_stack=1"
```

**Windows 支持**: MSVC 不支持 TSan,仅在 Linux CI 启用。Windows 通过 AddressSanitizer (ASan) 替代。

### 1.3 误报处理

已知误报场景:
- Qt 信号槽的 queued connection 内部状态
- QThreadPool 内部调度

**处理方式**: 通过 `TSAN_OPTIONS=suppressions=tsan_suppressions.txt` 抑制已知误报,文件位于项目根目录。

### 1.4 验收标准

- CI 中 TSan 任务通过,零未抑制的警告
- `tsan_suppressions.txt` 内容有注释说明每项的合理性

---

## 2. test_utils 既有失败修复

### 2.1 失败清单

v1.6.x 遗留 9 个测试失败:

| 测试 | 文件:行 | 失败原因 |
|------|---------|----------|
| `TestStringUtils::testTrimLeft` | test_utils.cpp:48 | `trimLeft` 实现错误地移除了右侧空格 |
| `TestStringUtils::testTrimRight` | test_utils.cpp:54 | `trimRight` 实现错误地移除了左侧空格 |
| `TestStringUtils::testStartsWith` | test_utils.cpp:75 | 空字符串匹配语义错误 |
| `TestFileUtils::testRemove` | test_utils.cpp:490 | `remove` 在 Windows 上对只读文件失败 |
| `TestFileUtils::testDirectory` | test_utils.cpp:511 | 根路径处理与期望不符 |
| `TestCompressUtils::testGzipCompressDecompress` | test_utils.cpp:598 | gzip 压缩对短数据反而变大 |
| `TestCompressUtils::testZlibCompressDecompress` | test_utils.cpp:606 | zlib 压缩对短数据反而变大 |
| `TestCompressUtils::testIsGzip` | test_utils.cpp:614 | gzip 魔数检测失败 |
| `TestCompressUtils::testIsZlib` | test_utils.cpp:621 | zlib 魔数检测失败 |

### 2.2 修复策略

**StringUtils**: 修复 `trimLeft`/`trimRight` 实现,确保只移除单侧空格;`startsWith` 空字符串应返回 true(标准语义)。

**FileUtils**: 
- `remove`: 在 Windows 上先清除只读属性再删除
- `directory`: 根路径返回 `/`(Linux)或 `C:\`(Windows),调整测试期望或实现

**CompressUtils**:
- 短数据压缩后变大是正常现象(压缩头开销),修复测试断言:验证 `decompress(compress(data)) == data` 而非 `compressed.size() < data.size()`
- `isGzip`/`isZlib`: 修复魔数检测逻辑

### 2.3 验收标准

- `test_utils.exe` 全部通过(0 failed)
- 修复不引入回归(其他测试套件不受影响)

---

## 3. tech_debt P2/P3 闭环

### 3.1 待闭环项

基于 `tech_debt_audit.md`,v1.6.x 已闭环 P0/P1,P2/P3 项待处理:

| # | 类别 | 文件 | 问题 | 优先级 |
|---|------|------|------|--------|
| 1 | 头文件 hygiene | `include/soul/async/future.h` | 不必要包含 `<QFuture>`/`<QFutureWatcher>`/`<QThreadPool>` | P2 |
| 2 | 头文件 hygiene | `include/soul/event/typed_event_bus.h` | 不必要包含 `thread_pool.h`/`logger.h` | P2 |
| 3 | 头文件 hygiene | `include/soul/storage/cache.h` | 未使用的 `<QHash>` | P3 |
| 4 | 头文件 hygiene | `include/soul/orm/query_wrapper.h` | `ISqlDialect` 应前向声明 | P3 |
| 5 | ADR-005 执行 | 全部公共类 | 文档化每个类的线程安全级别 | P3 |
| 6 | 自动化 | CI | 添加 Clang-Tidy 检查 blanket catch / raw new | P3 |
| 7 | 自动化 | CI | TSan 接入(见第 1 节) | P0 |

### 3.2 头文件 hygiene 修复模式

**前向声明模式**(以 `query_wrapper.h` 为例):

```cpp
// Before
#include "soul/orm/sql_dialect.h"
class QueryWrapper {
    ISqlDialect* m_dialect;
};

// After
namespace sc { namespace orm { class ISqlDialect; } }
class QueryWrapper {
    sc::orm::ISqlDialect* m_dialect;
};
```

**注意事项**:
- 模板代码中 `QFuture<T>` 按值使用,无法前向声明,需保留包含
- `QFutureWatcher<T>` 作为成员指针,可前向声明,但模板实例化时需要完整类型,实际收益有限

### 3.3 线程安全文档化

为每个公共类添加 Doxygen 注释,标注线程安全级别:

```cpp
/**
 * @brief 连接池,管理 HTTP 连接复用
 * @thread_safety Thread-Safe — 内部使用 std::mutex 同步所有访问
 * @see ADR-005
 */
class ConnectionPool : public QObject { ... };
```

### 3.4 验收标准

- P2 项全部修复,编译时间减少 ≥ 10%
- P3 项按时间允许推进,不阻塞 v1.7.0 发布
- Clang-Tidy 检查任务在 CI 中通过

---

## 4. 路线图文档同步

### 4.1 问题

`docs/14_roadmap.md` 严重过时:
- v1.0 仍标记 "In Development"(实际已发布)
- 缺少 v1.3/v1.4/v1.5/v1.6 版本
- 缺少 v1.7.0 规划
- 时间线(Q4 2024 等)与实际不符

### 4.2 修复方案

重写 `14_roadmap.md`:
1. v1.0-v1.6 标记为 "Released",附发布日期与关键交付物链接
2. v1.7.0 标记为 "In Development",链接到本目录
3. v1.8.0+ 列出长期愿景(SoulGateway / 分布式缓存 / OAuth2)
4. 时间线改为相对里程碑(M0/M1/M2)而非固定日期

### 4.3 验收标准

- `14_roadmap.md` 与实际版本一一对应
- 中英文版本同步更新(`docs/` 与 `docs_chinese/`)

---

## 5. 工作量估算

| 工作项 | 估算(人日) |
|--------|-------------|
| TSan CI 配置 + 误报处理 | 3 |
| test_utils 9 个失败修复 | 2 |
| 头文件 hygiene P2 | 2 |
| 头文件 hygiene P3 | 1 |
| 线程安全文档化 | 2 |
| Clang-Tidy CI 任务 | 1 |
| 路线图同步 | 1 |
| **合计** | **12** |

---

**文档状态**: Draft
**最后更新**: 2026-07-26
