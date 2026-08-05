# 05 — UI 组件库

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 5.1 组件清单 (30+)

### 基础组件

| 组件 | 文件 | 说明 |
|------|------|------|
| `BaseWidget` | `base_widget.h` | 所有 Widget 的基类 |
| `BaseView` | `base_view.h` | 视图基类 |
| `BaseViewModel` | `base_view_model.h` | ViewModel 基类 |
| `BaseDialog` | `base_dialog.h` | 对话框基类 |

### 容器组件

| 组件 | 文件 | 说明 |
|------|------|------|
| `Window` | `window.h` | 主窗口 |
| `Page` | `page.h` | 页面容器 |
| `Card` | `card.h` | 卡片容器 |
| `Sidebar` | `sidebar.h` | 侧边栏导航 |
| `SidebarHoverFilter` | `sidebar_hover_filter.h` | 侧边栏悬浮效果 |
| `Navigation` | `navigation.h` | 导航组件 |
| `TabBar` | `tab_bar.h` | 标签栏 |
| `GlassWidget` | `glass_widget.h` | 毛玻璃效果容器 |
| `GlassEffectCache` | `glass_effect_cache.h` | 毛玻璃效果缓存 |

### 表单控件

| 组件 | 文件 | 说明 |
|------|------|------|
| `Button` | `button.h` | 按钮 |
| `Input` | `input.h` | 输入框 |
| `Checkbox` | `checkbox.h` | 复选框 |
| `Switch` | `switch.h` | 开关 |
| `Slider` | `slider.h` | 滑块 |
| `Dropdown` | `dropdown.h` | 下拉选择 |
| `Progress` | `progress.h` | 进度条 |

### 反馈组件

| 组件 | 文件 | 说明 |
|------|------|------|
| `Toast` | `toast.h` | 轻提示 |
| `ToolTip` | `tool_tip.h` | 工具提示 |
| `Dialog` | `dialog.h` | 对话框 |
| `Loading` | `loading.h` | 加载中 |
| `Spinner` | `spinner.h` | 旋转加载 |
| `EmptyWidget` | `empty_widget.h` | 空状态 |
| `Badge` | `badge.h` | 徽标 |

### 装饰组件

| 组件 | 文件 | 说明 |
|------|------|------|
| `Avatar` | `avatar.h` | 头像 |
| `Icon` | `icon.h` | 图标 |
| `ScrollBar` | `scroll_bar.h` | 滚动条 |
| `Animation` | `animation.h` | 动画效果 |

### 主题系统

| 组件 | 文件 | 说明 |
|------|------|------|
| `Theme` | `theme.h` | 主题管理 (Singleton) |
| `Style` | `style.h` | 样式管理 |
| `DesignConstants` | `design_constants.h` | 设计常量 |
| `IconManager` | `icon_manager.h` | 图标管理 (Singleton) |

---

## 5.2 设计语言

- **风格**: iOS 风格毛玻璃效果 (iOS17 风格)
- **配色**: 现代极简轻奢紫白粉配色 (亮色) / 黑紫色调 (暗色)
- **统一 Design Token**: 通过 `Style` 类集中管理颜色、字体、圆角、间距、阴影

### Design Token 体系

| 维度 | 管理方式 | 对应类 |
|------|----------|--------|
| **颜色** | `ColorRole` 枚举 | `Style::color(ColorRole)` |
| **字体** | 统一返回 | `Style::font()` |
| **圆角** | `CornerRadius` 枚举 | `Style::cornerRadius(CornerRadius)` |
| **间距** | `Spacing` 枚举 | `Style::spacing(Spacing)` |
| **阴影** | 从主题颜色生成 | `Theme` |
| **动画** | 统一静态方法 | `Animation` |
| **图标** | 单例管理 | `IconManager` |

### 扩展颜色角色

| 角色 | 语义 | 亮色主题 | 暗色主题 |
|------|------|----------|----------|
| `Success` | 成功状态 | `#10b981` | `#34d399` |
| `GlassBackground` | 毛玻璃背景 | `#ffffff` | `#1e293b` |
| `GlassTint` | 毛玻璃着色层 | `rgba(28,28,30,65)` | `rgba(255,255,255,40)` |
| `GlowColor` | 发光效果 | `#6366f1` | `#818cf8` |

---

## 5.3 设计原则

1. **组件优先**: 任何使用 2 次及以上的 UI 必须抽象为公共组件
2. **单一职责**: 每个类只负责一件事 (UI 渲染 / 状态管理 / 业务逻辑 分离)
3. **统一设计语言**: 整个 UI 组件库保持一致的视觉和交互体验
4. **禁止硬编码**: 颜色、字号、间距等必须通过 Design Token 获取