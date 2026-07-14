#ifndef SOUL_UI_NAVIGATION_H
#define SOUL_UI_NAVIGATION_H

#include <QStackedWidget>
#include <QString>
#include <functional>

namespace sc {

/**
 * @class Navigation
 * @brief 页面导航管理器
 *
 * Navigation 提供静态的页面导航管理功能：
 * - push()：推入新页面
 * - pop()：返回上一页
 * - popToRoot()：返回根页面
 * - replace()：替换当前页面
 * - 页面栈管理和标题追踪
 *
 * 使用方式：
 * @code
 * Navigation::setRootWidget(stackedWidget);
 * Navigation::push(new HomePage(), "首页");
 * Navigation::pop();
 * @endcode
 *
 * @see Page
 */
class Navigation {
public:
    /**
     * @brief 设置根导航组件
     * @param stack QStackedWidget 指针
     */
    static void setRootWidget(QStackedWidget* stack);

    /**
     * @brief 推入新页面
     * @param widget 页面组件
     * @param title 页面标题（可选）
     */
    static void push(QWidget* widget, const QString& title = "");

    /**
     * @brief 返回上一页
     */
    static void pop();

    /**
     * @brief 返回根页面
     */
    static void popToRoot();

    /**
     * @brief 替换当前页面
     * @param widget 新页面组件
     * @param title 页面标题（可选）
     */
    static void replace(QWidget* widget, const QString& title = "");

    /**
     * @brief 获取当前页面索引
     * @return 当前索引
     */
    static int currentIndex();

    /**
     * @brief 获取当前页面组件
     * @return 当前页面 QWidget 指针
     */
    static QWidget* currentWidget();

    /**
     * @brief 获取当前页面标题
     * @return 当前页面标题
     */
    static QString currentTitle();

    /**
     * @brief 获取页面栈数量
     * @return 页面数量
     */
    static int stackCount();

    /**
     * @brief 导航回调类型
     * @param widget 当前页面组件
     * @param title 当前页面标题
     */
    using NavigationCallback = std::function<void(QWidget*, const QString&)>;

    /**
     * @brief 设置导航回调
     * @param callback 导航回调函数
     */
    static void setOnNavigate(NavigationCallback callback);

private:
    static QStackedWidget* s_stack;
    static QStringList s_titles;
    static NavigationCallback s_onNavigate;
};

}

#endif
