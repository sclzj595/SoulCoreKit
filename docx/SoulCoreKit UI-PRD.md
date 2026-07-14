SoulCoreKit UI 组件库产品需求文档（PRD）
版本：V1.0
状态：草稿
目标平台：Qt 6.5+ / C++17 / Qt Widgets + QSS (无 Qt Quick)
定位：SoulCoreKit 的核心 UI 模块，为所有桌面产品提供统一、精致、高交互感的原生界面组件。

一、设计哲学与核心原则
1.1 设计哲学：原生质感 · 沉浸交互
本 UI 组件库的设计严格遵循 “iOS 风格质感 + 桌面原生性能” 的哲学，所有视觉与交互设计围绕此核心展开。

视觉语言 (Visual Language)：以 毛玻璃 (Glassmorphism) 为底，发光边缘 (Glow Edge) 为饰，营造通透、高级的质感。

交互灵魂 (Interactive Soul)：以 “呼吸感 (Breathing)” 为魂，通过细腻的悬停、脉动和回弹动效，让界面充满生命力。

技术基石 (Technical Foundation)：纯 Qt Widgets + QSS。利用 Qt 的样式表机制实现绝大部分视觉风格，确保高性能和低复杂度，拒绝 Qt Quick。

1.2 核心原则
一致性 (Consistency)：所有组件遵循统一的主题、间距、圆角和动效参数，确保应用整体感。

可主题化 (Themable)：深色/浅色主题一键切换，所有颜色和效果参数均由主题系统驱动。

高性能 (Performance)：所有动画和效果在 60fps 下流畅运行，不引入额外的渲染开销。

易用性 (Usability)：组件接口设计直观，遵循 Qt 惯用法，开发者可通过简单的 QSS 或 API 进行定制。

二、技术选型与架构
2.1 核心技术栈
UI框架：Qt 6.5+ Widgets。完全基于 QWidget 体系。

样式技术：QSS (Qt Style Sheets)。所有组件的颜色、圆角、阴影、发光等视觉效果，优先通过 QSS 实现，以保持与 Qt 原生机制的一致性和高性能。

动画引擎：Qt 动画框架 (QPropertyAnimation, QParallelAnimationGroup)。用于实现呼吸、回弹、页面切换等动效。

依赖关系：UI 模块依赖 Core (主题、单例) 和 Utils (图标、图像) 模块。

2.2 架构思想
组合优于继承：优先通过组合现有 QWidget 子类并应用 QSS 来构建复杂组件。

配置驱动样式：参考 AntDesignQt5 的设计，组件的 QSS 样式字符串由主题系统动态生成和拼接，避免硬编码。

三、模块清单与功能需求
UI 组件库分为 “核心机制” 与 “基础组件” 两大部分。

3.1 核心机制 (Core Mechanisms)
模块	职责	关键技术点
主题系统 (Theme)	管理深色/浅色主题，提供颜色、间距、圆角、字体等设计令牌 (Design Tokens)。	从 JSON 配置文件加载主题数据；通过信号 themeChanged() 通知所有组件刷新 QSS。
毛玻璃效果 (Glass)	为 QWidget 提供毛玻璃背景的绘制能力。	方案：在组件 paintEvent 中，截取底层背景图像，应用 QGraphicsBlurEffect 或手动进行高斯模糊，再与半透明颜色混合绘制。需处理性能优化（如缓存模糊结果）。
动效引擎 (Animation)	提供呼吸、发光、回弹等标准动画效果的便捷 API。	封装 QPropertyAnimation，提供 applyBreathing(), applyGlow() 等静态方法，方便任何组件调用。
图标系统 (Icon)	统一管理内建 SVG 图标 (Font Awesome / Ant Design 图标集)。	IconManager 单例，提供 icon(name, size, color) 方法返回 QPixmap/QIcon。
3.2 基础组件清单 (UI Widgets)
组件名	功能描述	iOS毛玻璃质感与交互细节 (QSS+动画实现)
BaseWidget	所有 UI 组件的基类。	提供统一的背景、圆角、边框和阴影绘制接口。集成主题监听和动效管理。
BaseWindow	主窗口基类。	支持无边框、可拖拽、可调整大小。背景应用全屏毛玻璃效果。窗口激活/失活时，发光边缘亮度自动调整。
BaseDialog	模态对话框基类。	背景遮罩层应用毛玻璃效果。对话框本身带有发光边框和呼吸感。
Button (Push/Icon)	按钮组件。	QSS：正常态半透明背景；悬停态 (Hover) 边缘发光 (box-shadow 或边框发光)。
Animation：悬停时应用 1.2s 周期的呼吸脉动 (scale 微变)；点击时应用 150ms 的按压回弹动画。
Card	信息卡片容器。	QSS：应用毛玻璃背景 (background: rgba(...), backdrop-filter 模拟)；圆角 16px；悬停时边缘发光。
Animation：悬停时有轻微上浮和呼吸感。
SideBar	侧边导航栏。	QSS：毛玻璃背景，右侧边缘有发光分割线。
交互：菜单项悬停有发光和呼吸动效。
Toast	轻提示。	QSS：毛玻璃背景，圆角胶囊样式。
Animation：从顶部或底部滑入，停留后淡出。
Loading	加载指示器。	QSS：环形或点状动画，由 QSS 实现旋转/跳动。
交互：背后遮罩层应用毛玻璃效果。
Input / TextEdit	输入框/文本框。	QSS：聚焦时边框发光 (line-color, box-shadow)。
交互：输入框在聚焦时播放发光渐变动画。
Switch / Checkbox	开关/复选框。	QSS：自定义样式，模拟 iOS 风格开关滑块。
Animation：状态切换时有平滑的颜色过渡和滑动动画。
Slider	滑动条。	QSS：自定义滑块与滑轨样式。
交互：滑块拖拽时有发光轨迹和数值提示。
四、核心交互与视觉规范
4.1 主题系统参数 (Theme Specification)
主题系统提供以下设计令牌，驱动所有 QSS 样式：

类别	令牌示例	浅色主题值	深色主题值	说明
颜色	--glass-bg-primary	rgba(255,255,255,0.65)	rgba(28,28,30,0.75)	毛玻璃主背景色
--glow-color-primary	rgba(0,122,255,0.40)	rgba(10,132,255,0.50)	主发光颜色
--text-primary	#1C1C1E	#F5F5F7	主要文字颜色
尺寸	--radius-card	16px	16px	卡片圆角
--spacing-md	16px	16px	标准间距
动效	--duration-hover	200ms	200ms	悬停过渡时长
--duration-breathing	1200ms	1200ms	呼吸动画周期
4.2 毛玻璃效果实现规范 (Glassmorphism in QSS)
由于 QSS 本身不支持 backdrop-filter，我们需要在 C++ 层面实现效果，但通过 QSS 控制样式参数。

实现策略：

在 GlassWidget 的 paintEvent 中，获取父窗口或底层窗口的背景图像。
对图像应用模糊算法 (可使用 QGraphicsBlurEffect 或快速盒状模糊)。
将模糊后的图像作为背景绘制，并叠加一层半透明颜色 (由 QSS 的 background-color 控制)。
QSS 接口：为开发者提供类似 glass-opacity, glass-blur 的自定义属性，以便通过 QSS 调整效果。

4.3 交互动效规范 (Interaction & Animation)
所有动效通过 Qt 动画框架实现，与 QSS 状态 (:hover, :pressed) 协同工作。

动效类型	触发条件	效果参数	曲线与时长
发光显隐	鼠标悬停/焦点进入	发光透明度与范围变化	EaseInOut, 200ms
呼吸脉动	鼠标悬停	轻微缩放 (1.0 ↔ 1.02) 或透明度变化	正弦曲线 (Sine), 1.2s 循环
点击回弹	鼠标按下与释放	按下缩放 0.97，释放回弹至 1.0	弹簧曲线 (Spring), 300ms
页面切换	导航/页面替换	淡入+上滑/淡出+下滑	EaseInOut, 200-300ms
五、非功能需求与演进
5.1 质量与性能
性能：组件初始化不影响应用启动速度；动画流畅，CPU 占用合理。

测试：关键组件 (Button, Card) 有单元测试和 Demo 示例。

文档：每个组件有 QSS 样式示例和 C++ API 说明。

5.2 演进路线
V1.0：完成核心机制 (主题、毛玻璃、动效) 和 10+ 个基础组件 (Button, Card, Window, Dialog, Toast, Loading, Input, Switch, Slider, SideBar)。

V1.1：增加复杂组件 (Table, Tree, Tabs, Progress)。

V2.0：根据项目需求，增加图表、富文本编辑器等高级组件。

六、总结
本 PRD 明确了 SoulCoreKit UI 组件库 的定位：一个基于 Qt6 Widgets + QSS，提供 iOS 级毛玻璃质感与交互细节的桌面 UI 基础设施。它通过 QSS 和 Qt 动画框架，将视觉规范与交互动效紧密结合，确保开发者能以高效、原生、一致的方式构建出具有高级质感的应用程序。