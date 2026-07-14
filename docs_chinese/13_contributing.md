# SoulCoreKit 贡献指南

## 概述

本文档定义了 SoulCoreKit 的贡献工作流和标准。

---

## 行为准则

### 尊重

- 尊重所有贡献者
- 倾听不同意见
- 提供建设性反馈

### 质量

- 保持高代码质量
- 遵循现有模式
- 为新代码编写测试

### 透明

- 记录你的变更
- 解释你的推理
- 遵循审查流程

---

## 开始贡献

### 前置条件

- Qt 6.5+
- CMake 3.16+
- 支持 C++17 的编译器
- Git

### Fork 仓库

```bash
git clone https://github.com/yourusername/SoulCoreKit.git
cd SoulCoreKit
```

### 构建

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## 贡献工作流

### 1. 查找问题

- 查看标记为 "good first issue" 的问题
- 查看标记为 "help wanted" 的问题
- 在问题中讨论你的计划

### 2. 创建分支

```bash
git checkout -b feature/your-feature-name
```

### 3. 进行更改

- 遵循编码风格指南
- 编写测试
- 更新文档

### 4. 提交

#### 提交消息格式

```
<type>(<scope>): <description>

<可选正文>

<可选页脚>
```

#### 类型

| 类型 | 描述 |
|------|-------------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `docs` | 文档变更 |
| `style` | 代码风格变更 |
| `refactor` | 重构 |
| `test` | 测试变更 |
| `chore` | 构建/CI 变更 |

#### 示例

```
feat(ui): 为 Button 添加呼吸动画

- 实现基于正弦的呼吸动画
- 添加动画缓存防止叠加
- 更新 Button QSS 实现发光效果

Closes #123
```

### 5. 推送

```bash
git push origin feature/your-feature-name
```

### 6. 创建 Pull Request

- 标题: 与提交消息相同
- 描述: 解释变更内容
- 引用相关问题
- 请求维护者审查

### 7. 审查流程

- 处理审查评论
- 必要时更新测试
- 等待批准

### 8. 合并

- Squash and merge
- 删除分支

---

## 代码审查指南

### 审查者

- 检查代码质量
- 验证测试覆盖率
- 确保 API 兼容性
- 遵循设计原则

### 贡献者

- 响应评论
- 做出请求的更改
- 必要时重新基址

---

## 测试

### 运行测试

```bash
cd build
ctest -v
```

### 测试要求

- 新代码必须有单元测试
- 测试必须在所有平台通过
- 覆盖率必须达到目标

---

## 文档

### Doxygen

- 所有公共 API 必须有 Doxygen 注释
- 遵循文档标准
- 在本地构建文档进行验证

### 更新文档

- 更新相关文档文件
- 添加使用示例
- 更新 changelog

---

## 许可证

### 许可证

SoulCoreKit 使用 MIT 许可证。

### 贡献者许可协议

通过贡献，你同意你的贡献将使用相同的许可证。

---

## 问题

- 创建 issue 提问
- 参与讨论
- 向维护者寻求帮助

---

## 贡献审查检查清单

- ☑ 分支遵循命名约定
- ☑ 提交消息遵循格式
- ☑ 代码遵循编码风格
- ☑ 添加/更新了测试
- ☑ 更新了文档
- ☑ 更新了 Changelog
- ☑ 所有测试通过
- ☑ API 兼容性已验证
- ☑ Pull request 描述清晰
- ☑ 审查评论已处理
