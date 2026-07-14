# SoulCoreKit 测试规范

## 概述

本文档定义了 SoulCoreKit 的测试策略，包括测试类型、覆盖率要求和测试基础设施。

---

## 测试类型

### 单元测试

- **范围**: 单个类和函数
- **隔离**: 对依赖使用 mock
- **框架**: Qt Test
- **位置**: `tests/unit/`

### 集成测试

- **范围**: 多个组件协同工作
- **示例**: Network + Storage 工作流
- **位置**: `tests/integration/`

### UI 测试

- **范围**: UI 组件和交互
- **框架**: Qt Test 配合 QWidget 测试
- **位置**: `tests/ui/`

### 回归测试

- **范围**: Bug 修复和向后兼容性
- **触发**: 每个 PR 都运行
- **位置**: `tests/regression/`

---

## 测试覆盖率要求

| 模块 | 目标覆盖率 | 状态 |
|--------|----------------|--------|
| **Core** | >90% | 必需 |
| **Base** | >80% | 必需 |
| **Logging** | >90% | 必需 |
| **Configuration** | >80% | 必需 |
| **Network** | >70% | 必需 |
| **Storage** | >80% | 必需 |
| **Async** | >70% | 必需 |
| **Event** | >80% | 必需 |
| **UI** | >70% | 必需 |
| **Auth** | >70% | 必需 |
| **Utils** | >80% | 必需 |

---

## 测试结构

### 目录布局

```
tests/
├── CMakeLists.txt
├── unit/
│   ├── core/
│   ├── ui/
│   ├── network/
│   └── ...
├── integration/
│   ├── network_storage/
│   └── ...
├── ui/
│   ├── button_test.cpp
│   ├── card_test.cpp
│   └── ...
└── regression/
    ├── issue_123_test.cpp
    └── ...
```

### 测试文件命名

- **规则**: `<component>_test.cpp`
- **示例**: `button_test.cpp`, `http_client_test.cpp`

---

## 测试框架

### Qt Test

```cpp
#include <QtTest/QtTest>
#include "soul/core/result.h"

class ResultTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testOk();
    void testErr();
    void testMap();
    void testFlatMap();
};

void ResultTest::testOk() {
    auto result = sc::Result<int>::Ok(42);
    QVERIFY(result.isOk());
    QCOMPARE(result.value(), 42);
}

void ResultTest::testErr() {
    auto error = sc::Error(sc::ErrorCode::NotFound, "Not found");
    auto result = sc::Result<int>::Err(error);
    QVERIFY(result.isErr());
    QCOMPARE(result.error().code(), sc::ErrorCode::NotFound);
}

QTEST_APPLESS_MAIN(ResultTest)
#include "result_test.moc"
```

### Mocking

使用依赖注入进行 mocking：

```cpp
class MockNetworkClient : public sc::INetworkClient {
public:
    MOCK_METHOD(Result<HttpResponse>, get, (const QString& url), (override));
};
```

---

## 测试执行

### CMake 集成

```cmake
enable_testing()

add_executable(result_test
    tests/unit/core/result_test.cpp
)

target_link_libraries(result_test PRIVATE
    Qt6::Test
    soul_core
)

add_test(NAME result_test COMMAND result_test)
```

### 运行测试

```bash
# 构建测试
cmake --build build --target all

# 运行所有测试
cd build
ctest

# 运行特定测试
ctest -R result_test

# 运行并显示详细输出
ctest -v
```

---

## 测试指南

### 测试内容

- **正常路径**: 预期行为
- **边界情况**: 边界条件
- **错误路径**: 错误处理
- **并发**: 线程安全性
- **性能**: 时间敏感操作

### 不需要测试的内容

- **第三方代码**: 信任 Qt 和标准库
- **实现细节**: 测试行为，而非实现
- **UI 美观性**: 关注功能，而非视觉外观

### 测试隔离

- 每个测试应独立
- 使用 `init()`/`cleanup()` 进行设置/清理
- 避免测试间共享状态

---

## CI/CD 集成

### 持续集成

- **平台**: Windows, macOS, Linux
- **触发**: Pull requests, main 分支推送
- **步骤**:
  1. 构建
  2. 运行测试
  3. 生成覆盖率报告
  4. 静态分析

### 覆盖率报告

- **工具**: gcov / lcov
- **格式**: HTML
- **阈值**: 如果覆盖率低于目标则失败

---

## 测试审查检查清单

- ☑ 单元测试覆盖正常路径
- ☑ 单元测试覆盖边界情况
- ☑ 单元测试覆盖错误路径
- ☑ 集成测试覆盖跨模块工作流
- ☑ UI 测试覆盖组件交互
- ☑ 回归测试覆盖 Bug 修复
- ☑ 测试正确隔离
- ☑ 覆盖率达到目标要求
- ☑ 测试在所有平台通过
- ☑ CI/CD 流水线在每个 PR 上运行测试
