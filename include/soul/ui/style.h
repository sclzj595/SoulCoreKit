#ifndef SOUL_UI_STYLE_H
#define SOUL_UI_STYLE_H

#include <QColor>
#include <QString>
#include <QFont>

namespace sc {

/**
 * @enum ColorRole
 * @brief 颜色角色枚举
 */
enum class ColorRole {
    Primary,          ///< 主色
    PrimaryLight,     ///< 主色浅色
    PrimaryDark,      ///< 主色深色
    Secondary,        ///< 次要色
    SecondaryLight,   ///< 次要色浅色
    SecondaryDark,    ///< 次要色深色
    Background,       ///< 背景色
    Surface,          ///< 表面色
    SurfaceVariant,   ///< 表面变体色
    Error,            ///< 错误色
    ErrorContainer,   ///< 错误容器色
    Success,          ///< 成功状态色
    OnPrimary,        ///< 主色上的文字颜色
    OnSecondary,      ///< 次要色上的文字颜色
    OnBackground,     ///< 背景上的文字颜色
    OnSurface,        ///< 表面上的文字颜色
    OnSurfaceVariant, ///< 表面变体上的文字颜色
    OnError,          ///< 错误上的文字颜色
    TextPrimary,      ///< 主要文字色
    TextSecondary,    ///< 次要文字色
    TextTertiary,     ///< 第三文字色
    Border,           ///< 边框色
    BorderLight,      ///< 浅色边框色
    Shadow,           ///< 阴影色
    Overlay,          ///< 覆盖层色
    GlassBackground,  ///< 毛玻璃背景色
    GlassTint,        ///< 毛玻璃着色层
    GlowColor         ///< 发光颜色
};

/**
 * @enum CornerRadius
 * @brief 圆角大小枚举
 */
enum class CornerRadius {
    None,        ///< 无圆角
    Small,       ///< 小圆角
    Medium,      ///< 中圆角
    Large,       ///< 大圆角
    ExtraLarge,  ///< 超大圆角
    Full         ///< 完全圆角
};

/**
 * @enum Spacing
 * @brief 间距大小枚举
 */
enum class Spacing {
    None,        ///< 无间距
    Tiny,        ///< 极小间距
    Small,       ///< 小间距
    Medium,      ///< 中间距
    Large,       ///< 大间距
    ExtraLarge   ///< 超大间距
};

/**
 * @class Style
 * @brief 样式管理类
 *
 * Style 管理应用的样式属性，包括颜色、圆角、间距和字体。
 * 支持通过 ColorRole、CornerRadius、Spacing 枚举来获取对应的样式值。
 *
 * 使用方式：
 * @code
 * Style style;
 * QColor primary = style.color(ColorRole::Primary);
 * int radius = style.cornerRadius(CornerRadius::Medium);
 * @endcode
 *
 * @see ColorRole, CornerRadius, Spacing, Theme
 */
class Style {
public:
    /**
     * @brief 构造函数
     */
    Style();

    /**
     * @brief 获取颜色
     * @param role 颜色角色
     * @return 颜色
     */
    QColor color(ColorRole role) const;

    /**
     * @brief 设置颜色
     * @param role 颜色角色
     * @param color 颜色值
     */
    void setColor(ColorRole role, const QColor& color);

    /**
     * @brief 获取圆角大小
     * @param radius 圆角类型
     * @return 圆角大小（像素）
     */
    int cornerRadius(CornerRadius radius) const;

    /**
     * @brief 设置圆角大小
     * @param radius 圆角类型
     * @param value 圆角大小（像素）
     */
    void setCornerRadius(CornerRadius radius, int value);

    /**
     * @brief 获取间距大小
     * @param space 间距类型
     * @return 间距大小（像素）
     */
    int spacing(Spacing space) const;

    /**
     * @brief 设置间距大小
     * @param space 间距类型
     * @param value 间距大小（像素）
     */
    void setSpacing(Spacing space, int value);

    /**
     * @brief 获取默认字体
     * @return 默认字体
     */
    QFont font() const;

    /**
     * @brief 设置默认字体
     * @param font 字体
     */
    void setFont(const QFont& font);

    /**
     * @brief 获取粗体字体
     * @return 粗体字体
     */
    QFont boldFont() const;

    /**
     * @brief 获取小号字体
     * @return 小号字体
     */
    QFont smallFont() const;

    /**
     * @brief 获取大号字体
     * @return 大号字体
     */
    QFont largeFont() const;

private:
    struct {
        QColor primary;
        QColor primaryLight;
        QColor primaryDark;
        QColor secondary;
        QColor secondaryLight;
        QColor secondaryDark;
        QColor background;
        QColor surface;
        QColor surfaceVariant;
        QColor error;
        QColor errorContainer;
        QColor success;
        QColor onPrimary;
        QColor onSecondary;
        QColor onBackground;
        QColor onSurface;
        QColor onSurfaceVariant;
        QColor onError;
        QColor textPrimary;
        QColor textSecondary;
        QColor textTertiary;
        QColor border;
        QColor borderLight;
        QColor shadow;
        QColor overlay;
        QColor glassBackground;
        QColor glassTint;
        QColor glowColor;
    } m_colors;

    struct {
        int none;
        int small;
        int medium;
        int large;
        int extraLarge;
        int full;
    } m_cornerRadii;

    struct {
        int none;
        int tiny;
        int small;
        int medium;
        int large;
        int extraLarge;
    } m_spacings;

    QFont m_font;
};

}

#endif
