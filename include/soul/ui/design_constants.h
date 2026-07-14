#ifndef SOUL_UI_DESIGN_CONSTANTS_H
#define SOUL_UI_DESIGN_CONSTANTS_H

#include <QtGlobal>

namespace sc {

namespace design {

// 默认圆角半径（像素）- 用于控件的圆角设计
constexpr int DEFAULT_BORDER_RADIUS = 16;
// 默认模糊半径（像素）- 用于毛玻璃效果
constexpr int DEFAULT_BLUR_RADIUS = 15;
// 默认不透明度 - 用于半透明效果（0.0-1.0）
constexpr qreal DEFAULT_OPACITY = 0.65;

// 窗口背景模糊半径（像素）- 用于窗口级别的毛玻璃效果
constexpr int WINDOW_BLUR_RADIUS = 20;

// 按钮内边距（像素）- 小号按钮
constexpr int BUTTON_SMALL_PADDING = 6;
// 按钮内边距（像素）- 中号按钮
constexpr int BUTTON_MEDIUM_PADDING = 8;
// 按钮内边距（像素）- 大号按钮
constexpr int BUTTON_LARGE_PADDING = 12;

// 按钮字体大小（像素）- 小号按钮
constexpr int BUTTON_SMALL_FONT_SIZE = 12;
// 按钮字体大小（像素）- 中号按钮
constexpr int BUTTON_MEDIUM_FONT_SIZE = 14;
// 按钮字体大小（像素）- 大号按钮
constexpr int BUTTON_LARGE_FONT_SIZE = 16;

// 按钮图标大小（像素）- 小号按钮
constexpr int BUTTON_SMALL_ICON_SIZE = 14;
// 按钮图标大小（像素）- 中号按钮
constexpr int BUTTON_MEDIUM_ICON_SIZE = 16;
// 按钮图标大小（像素）- 大号按钮
constexpr int BUTTON_LARGE_ICON_SIZE = 20;

// Toast 水平内边距（像素）
constexpr int TOAST_PADDING_HORIZONTAL = 16;
// Toast 垂直内边距（像素）
constexpr int TOAST_PADDING_VERTICAL = 12;
// Toast 字体大小（像素）
constexpr int TOAST_FONT_SIZE = 14;
// Toast 图标大小（像素）
constexpr int TOAST_ICON_SIZE = 20;
// Toast 默认显示时长（毫秒）
constexpr int TOAST_DEFAULT_DURATION = 3000;

// 动画持续时间常量 短动画
constexpr int ANIMATION_DURATION_SHORT = 150;
// 动画持续时间（毫秒）- 正常动画
constexpr int ANIMATION_DURATION_NORMAL = 200;
// 动画持续时间（毫秒）- 长动画
constexpr int ANIMATION_DURATION_LONG = 300;
// 动画持续时间（毫秒）- 呼吸动画周期
constexpr int ANIMATION_DURATION_BREATHING = 1200;

// 悬停抬起偏移量（像素）- HoverLift 动画的垂直位移
constexpr int HOVER_LIFT_OFFSET = 4;
// 按压缩放因子（百分比）- 按压时的缩放比例，97 表示 97%
constexpr int PRESS_SCALE_FACTOR = 97;
// 缩放动画起始比例（0.0-1.0）- scaleUp/scaleDown 动画的起始缩放比例
constexpr qreal SCALE_ANIMATION_START_FACTOR = 0.8;
// 呼吸动画缩放因子（0.0-1.0）- 呼吸动画的目标缩放比例
constexpr qreal BREATHING_SCALE_FACTOR = 1.02;
// 震动动画偏移量（像素）- shake 动画的左右偏移量
constexpr int SHAKE_OFFSET = 5;
// 震动动画结束偏移量（像素）- shake 动画最后阶段的偏移量
constexpr int SHAKE_END_OFFSET = 3;
// Glow 效果最大模糊半径（像素）
constexpr int GLOW_MAX_BLUR_RADIUS = 15;
// 默认图标大小（像素）- Icon/IconManager 的默认图标尺寸
constexpr int DEFAULT_ICON_SIZE = 24;

// Card 设计常量
// 卡片内容边距（像素）
constexpr int CARD_CONTENT_MARGIN = 16;
// 卡片内容间距（像素）- 卡片内部元素之间的间距
constexpr int CARD_CONTENT_SPACING = 8;

}

}

#endif
