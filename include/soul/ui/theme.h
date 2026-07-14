#ifndef SOUL_UI_THEME_H
#define SOUL_UI_THEME_H

#include <QObject>
#include <memory>
#include "style.h"
#include "soul/core/singleton.h"

namespace sc {

/**
 * @enum ThemeMode
 * @brief 主题模式枚举
 */
enum class ThemeMode {
    Light,   ///< 浅色主题
    Dark,    ///< 深色主题
    System   ///< 跟随系统主题
};

/**
 * @class Theme
 * @brief 主题管理器（单例）
 *
 * Theme 管理应用的主题设置，支持浅色/深色/系统主题切换。
 * 使用 Style 对象管理颜色、圆角和间距等样式属性。
 *
 * 使用方式：
 * @code
 * Theme::instance().setMode(ThemeMode::Dark);
 * Theme::instance().applyToApp();
 * @endcode
 *
 * @see Style, ThemeMode
 */
class Theme : public QObject, public Singleton<Theme> {
    Q_OBJECT
    friend class Singleton<Theme>;
public:
    /**
     * @brief 初始化主题管理器
     */
    void init();

    /**
     * @brief 关闭主题管理器
     */
    void shutdown();

    /**
     * @brief 设置主题模式
     * @param mode 主题模式
     */
    void setMode(ThemeMode mode);

    /**
     * @brief 获取当前主题模式
     * @return 当前主题模式
     */
    ThemeMode mode() const;

    /**
     * @brief 获取样式（const）
     * @return Style 引用
     */
    const Style& style() const;

    /**
     * @brief 获取样式
     * @return Style 引用
     */
    Style& style();

    /**
     * @brief 将主题应用到整个应用
     */
    void applyToApp();

    /**
     * @brief 检测系统是否使用深色模式
     * @return 如果系统使用深色模式返回 true
     */
    static bool isSystemDarkMode();

    /**
     * @brief 保存当前主题设置到配置
     */
    void saveThemeMode();

    /**
     * @brief 从配置恢复主题设置
     */
    void restoreThemeMode();

signals:
    /**
     * @brief 主题变化信号
     */
    void themeChanged();

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    Theme();

    /**
     * @brief 析构函数
     */
    ~Theme() override = default;

    /**
     * @brief 加载浅色主题
     */
    void loadLightTheme();

    /**
     * @brief 加载深色主题
     */
    void loadDarkTheme();

    /**
     * @brief 加载系统主题（跟随系统自动切换明暗）
     */
    void loadSystemTheme();

    /**
     * @brief 处理系统主题变化
     */
    void onSystemThemeChanged();

    ThemeMode m_mode;
    Style m_style;
};

}

#endif
