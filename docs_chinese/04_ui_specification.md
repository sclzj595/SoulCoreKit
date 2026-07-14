# SoulCoreKit UI 规范

## 概述

本文档指定了 SoulCoreKit UI 组件库的视觉设计、交互模式和技术实现标准。

---

## 设计理念

### 原生品质 · 沉浸式交互

UI 组件库严格遵循"iOS 级品质 + 桌面原生性能"的设计理念。所有视觉和交互设计都围绕这一核心展开。

### 视觉语言

- **毛玻璃基底**：透明、高品质质感
- **发光边缘点缀**：微妙的发光效果用于强调
- **一致的设计令牌**：颜色、间距、圆角统一管理

### 交互灵魂

- **呼吸效果**：微妙的悬停、脉冲和弹跳动画
- **平滑过渡**：60fps 动画性能
- **即时反馈**：对用户操作的清晰视觉响应

### 技术基础

- **纯 Qt Widgets + QSS**：不依赖 Qt Quick
- **配置驱动样式**：从主题系统动态生成 QSS
- **组合优于继承**：通过组合简单组件构建复杂组件

---

## 设计令牌

### 颜色系统

| 颜色角色 | 语义 | 亮色主题 | 暗色主题 |
|----------|------|----------|----------|
| `Primary` | 主品牌色 | `#6366f1` | `#818cf8` |
| `PrimaryHover` | 悬停主色 | `#818cf8` | `#a5b4fc` |
| `Secondary` | 次要色 | `#64748b` | `#94a3b8` |
| `Success` | 成功状态 | `#10b981` | `#34d399` |
| `Warning` | 警告状态 | `#f59e0b` | `#fbbf24` |
| `Error` | 错误状态 | `#ef4444` | `#f87171` |
| `Info` | 信息 | `#3b82f6` | `#60a5fa` |
| `TextPrimary` | 主要文字 | `#1c1c1e` | `#f5f5f7` |
| `TextSecondary` | 次要文字 | `#86868b` | `#8e8e93` |
| `TextTertiary` | 三级文字 | `#aeaeaf` | `#636366` |
| `Background` | 主背景 | `#ffffff` | `#000000` |
| `Surface` | 表面背景 | `#f2f2f7` | `#1c1c1e` |
| `GlassBackground` | 毛玻璃背景 | `#ffffff` | `#1e293b` |
| `GlassTint` | 毛玻璃着色 | `rgba(28,28,30,65)` | `rgba(255,255,255,40)` |
| `GlowColor` | 发光效果 | `#6366f1` | `#818cf8` |
| `Border` | 边框色 | `#e5e5ea` | `#38383a` |
| `Divider` | 分隔线色 | `#f2f2f7` | `#2c2c2e` |

### 排版

| 令牌 | 值 | 描述 |
|------|-----|------|
| `FontFamily` | 系统默认 | 平台原生字体 |
| `FontSizeXS` | 10px | 特小文字 |
| `FontSizeSM` | 12px | 小文字 |
| `FontSizeMD` | 14px | 普通文字 |
| `FontSizeLG` | 16px | 大文字 |
| `FontSizeXL` | 18px | 特大文字 |
| `FontSizeXXL` | 24px | 标题文字 |
| `FontWeightRegular` | 400 | 常规粗细 |
| `FontWeightMedium` | 500 | 中等粗细 |
| `FontWeightSemibold` | 600 | 半粗体 |

### 间距

| 令牌 | 值 | 描述 |
|------|-----|------|
| `SpacingXS` | 4px | 特小间距 |
| `SpacingSM` | 8px | 小间距 |
| `SpacingMD` | 16px | 普通间距 |
| `SpacingLG` | 24px | 大间距 |
| `SpacingXL` | 32px | 特大间距 |
| `SpacingXXL` | 48px | 区块间距 |

### 圆角

| 令牌 | 值 | 描述 |
|------|-----|------|
| `RadiusNone` | 0px | 直角 |
| `RadiusSM` | 8px | 小圆角 |
| `RadiusMD` | 12px | 普通圆角 |
| `RadiusLG` | 16px | 大圆角 |
| `RadiusXL` | 24px | 特大圆角 |
| `RadiusFull` | 9999px | 药丸形 |

### 阴影

| 令牌 | 值 | 描述 |
|------|-----|------|
| `ShadowSM` | `0 2px 4px rgba(0,0,0,0.05)` | 小阴影 |
| `ShadowMD` | `0 4px 12px rgba(0,0,0,0.08)` | 普通阴影 |
| `ShadowLG` | `0 8px 24px rgba(0,0,0,0.12)` | 大阴影 |
| `ShadowGlow` | `0 0 20px rgba(99,102,241,0.3)` | 发光效果 |

### 动画

| 令牌 | 值 | 描述 |
|------|-----|------|
| `DurationFast` | 100ms | 快速过渡 |
| `DurationNormal` | 200ms | 普通过渡 |
| `DurationSlow` | 300ms | 慢速过渡 |
| `DurationBreathing` | 1200ms | 呼吸周期 |
| `EasingStandard` | `QEasingCurve::InOutQuad` | 标准缓动 |
| `EasingSpring` | `QEasingCurve::OutElastic` | 弹簧效果 |
| `EasingSine` | `QEasingCurve::InOutSine` | 平滑正弦 |

---

## 毛玻璃实现

### 策略

由于 QSS 不原生支持 `backdrop-filter`，我们在 C++ 中实现毛玻璃效果，同时通过 QSS 控制参数。

### 实现步骤

1. 在 `GlassWidget::paintEvent()` 中捕获底层窗口背景
2. 使用 `QGraphicsBlurEffect` 应用高斯模糊
3. 将模糊后的图像绘制为背景
4. 叠加由 QSS 控制的半透明颜色

### QSS 自定义属性

| 属性 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| `glass-opacity` | double | 0.65 | 背景不透明度 |
| `glass-blur` | int | 20 | 模糊半径（像素） |
| `glass-tint-color` | color | rgba(255,255,255,0.3) | 着色颜色 |

### 性能优化

- 静态背景缓存模糊结果
- 仅在位置/大小变化时重绘
- 使用较低模糊半径的 `QGraphicsBlurEffect` 以获得更好性能

---

## 动画规范

### 动画类型

| 类型 | 触发条件 | 效果 | 曲线 | 时长 |
|------|----------|------|------|------|
| **Glow** | 悬停 / 聚焦 | 发光不透明度和范围变化 | InOutQuad | 200ms |
| **Breathing** | 悬停 | 轻微缩放（1.0 ↔ 1.02） | Sine | 1200ms 循环 |
| **Bounce** | 按下 / 释放 | 按下缩放 0.97，释放回弹到 1.0 | OutElastic | 300ms |
| **Fade** | 页面过渡 | 不透明度变化 | InOutQuad | 200-300ms |
| **Slide** | 页面过渡 | 位置变化 | InOutQuad | 200-300ms |
| **Scale** | 出现 / 消失 | 从 0.9 缩放到 1.0 | OutBack | 300ms |

### 动画状态

```
Idle → Hover → Pressed → Idle
   ↑                   ↓
   └───────────────────┘
```

### 动画缓存

- 每个组件维护动画缓存以防止堆叠
- 新动画开始时，停止同类型的先前动画
- 使用 `QAnimationGroup` 进行协调多属性动画

---

## 组件规范

### Button

**状态**：
- Normal：半透明背景
- Hover：发光边缘 + 呼吸脉冲
- Pressed：缩放 0.97 + 回弹
- Disabled：降低不透明度

**QSS 结构**：
```qss
Button {
    background-color: rgba(99, 102, 241, 0.1);
    color: #6366f1;
    border-radius: 12px;
    padding: 8px 16px;
}
Button:hover {
    background-color: rgba(99, 102, 241, 0.2);
}
Button:pressed {
    background-color: rgba(99, 102, 241, 0.3);
}
Button:disabled {
    opacity: 0.5;
}
```

### Card

**状态**：
- Normal：毛玻璃背景 + 阴影
- Hover：抬起效果 + 发光边缘 + 呼吸

**QSS 结构**：
```qss
Card {
    background-color: rgba(255, 255, 255, 0.65);
    border-radius: 16px;
    padding: 16px;
}
```

### Input

**状态**：
- Normal：边框 + 背景
- Focus：发光边框 + 阴影

**QSS 结构**：
```qss
Input {
    background-color: rgba(255, 255, 255, 0.8);
    border: 1px solid #e5e5ea;
    border-radius: 12px;
    padding: 10px 14px;
}
Input:focus {
    border-color: #6366f1;
}
```

### Window

**特性**：
- 无边框 + 自定义标题栏
- 可拖拽
- 可调整大小
- 毛玻璃背景
- 激活/失活发光调整

---

## 主题系统

### 主题类型

| 主题 | 描述 |
|------|------|
| `Light` | 亮色配色方案 |
| `Dark` | 暗色配色方案 |
| `System` | 跟随系统偏好 |

### 主题切换

1. `Theme::setCurrentTheme(ThemeType)`
2. 发出 `themeChanged()` 信号
3. 所有组件监听并更新 QSS
4. 设置通过 `Storage::Settings` 持久化

### 系统主题适配

- Windows：监控注册表主题变化
- macOS：使用 `NSWorkspace` 通知
- Linux：监控 `GTK_THEME` 环境变量

---

## 布局系统

### 布局原则

- 使用 Qt 内置布局管理器（`QVBoxLayout`、`QHBoxLayout`、`QGridLayout`）
- 应用设计令牌进行间距
- 通过 `QSizePolicy` 实现响应式设计

### 间距规则

| 布局类型 | 间距 |
|----------|------|
| 表单布局 | `SpacingSM` |
| 卡片内容 | `SpacingMD` |
| 页面布局 | `SpacingLG` |
| 区块布局 | `SpacingXL` |

---

## 可访问性

### 要求

- 所有组件必须支持键盘导航
- 焦点指示器必须可见
- 支持高对比度模式
- 屏幕阅读器兼容性

---

## 性能要求

- 组件初始化：每个组件 < 50ms
- 动画性能：最低 60fps
- 内存使用：基本 UI 模块 < 10MB
- 绘制时间：每帧 < 16ms

---

## 跨平台考虑

| 平台 | 特殊考虑 |
|------|----------|
| **Windows** | DPI 缩放、Acrylic 效果支持 |
| **macOS** | Vibrancy 效果、圆角 |
| **Linux** | GTK 主题集成、Wayland 支持 |

---

## UI 审查清单

- ☑ 使用设计令牌而非硬编码值
- ☑ 遵循动画规范
- ☑ 正确实现毛玻璃效果
- ☑ 支持亮色和暗色主题
- ☑ 有正确的悬停/按下/禁用状态
- ☑ 满足性能要求
- ☑ 支持键盘导航
- ☑ 遵循布局间距规则
- ☑ 在所有目标平台上正常工作
- ☑ 有视觉状态的单元测试