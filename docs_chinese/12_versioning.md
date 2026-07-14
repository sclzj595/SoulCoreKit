# SoulCoreKit 版本策略

## 概述

本文档定义了 SoulCoreKit 的版本策略，基于语义化版本（SemVer）2.0。

---

## 语义化版本

### 格式

```
MAJOR.MINOR.PATCH
```

### MAJOR 版本

- **递增**: 破坏性 API 变更
- **兼容性**: 不向后兼容
- **示例**:
  - 删除公共 API
  - 更改方法签名
  - 重命名类或函数
  - 删除枚举值

### MINOR 版本

- **递增**: 新功能，向后兼容
- **兼容性**: 完全向后兼容
- **示例**:
  - 添加新的公共类
  - 向现有类添加新方法
  - 添加新的枚举值
  - 添加带默认值的新参数
  - 弃用（但不删除）现有 API

### PATCH 版本

- **递增**: 仅 Bug 修复
- **兼容性**: 完全向后兼容
- **示例**:
  - 修复 Bug
  - 改进性能
  - 修复文档

---

## API 稳定性

### 公共 API

**定义**: `include/soul/` 目录中暴露的所有类、函数和常量。

**保证**: 公共 API 在 **次要版本** 间保持兼容。

**示例**:
```
1.0 → 1.1 → 1.2: 向后兼容
1.2 → 2.0: 允许破坏性变更
```

### ABI 稳定性

**定义**: 动态链接库的二进制接口。

**保证**: ABI 在 **次要版本** 间保持稳定。

**禁止的变更**:
- 重排类成员
- 更改虚函数表布局
- 更改导出类的大小
- 删除导出符号

---

## 版本查询 API

### 运行时版本检查

```cpp
#include "soul/core/version.h"

int major = sc::Version::major();      // 1
int minor = sc::Version::minor();      // 0
int patch = sc::Version::patch();      // 0
QString version = sc::Version::toString(); // "1.0.0"
```

### 编译时版本检查

```cpp
#if SOUL_CORE_VERSION >= SOUL_CORE_VERSION_CHECK(1, 1, 0)
    // 使用新功能
#endif
```

---

## 发布流程

### 发布前检查清单

- [ ] 所有测试通过
- [ ] 文档已更新
- [ ] API 兼容性已验证
- [ ] `version.h` 中的版本号已更新
- [ ] Changelog 已更新

### 发布分支

```bash
git checkout -b release/1.0.0
```

### 打标签

```bash
git tag -a v1.0.0 -m "Version 1.0.0"
git push origin v1.0.0
```

### Changelog

每个发布必须有 changelog 条目：

```markdown
## 1.0.0 (2024-01-01)

### Added
- 初始发布
- 包含 Result<T> 和 Error 类型的 Core 模块
- 包含玻璃态控件的 UI 模块
- 包含 HTTP 客户端的 Network 模块

### Fixed
- 无

### Deprecated
- 无
```

---

## 弃用策略

### 标记弃用

- 使用 `[[deprecated]]` 属性
- 添加 `@deprecated` Doxygen 标签
- 提供迁移指南

**示例**:
```cpp
/**
 * @deprecated 使用 newMethod() 代替
 */
[[deprecated("Use newMethod() instead")]]
void oldMethod();
```

### 删除

- 弃用的 API 必须保留至少一个主要版本
- 删除必须在发布说明中宣布
- 必须提供迁移指南

---

## 兼容性矩阵

| 版本 | Qt 6.5 | Qt 6.6 | Qt 6.7 |
|---------|--------|--------|--------|
| 1.0.x | ✅ | ✅ | ✅ |
| 1.1.x | ✅ | ✅ | ✅ |
| 2.0.x | ❌ | ✅ | ✅ |

---

## 版本审查检查清单

- ☑ 版本遵循 SemVer 格式
- ☑ MAJOR 版本在破坏性变更时递增
- ☑ MINOR 版本在添加新功能时递增
- ☑ PATCH 版本在 Bug 修复时递增
- ☑ 公共 API 在次要版本间保持兼容
- ☑ ABI 在次要版本间保持稳定
- ☑ 版本 API 是最新的
- ☑ Changelog 已更新
- ☑ 弃用的 API 已正确标记
- ☑ 遵循发布流程
