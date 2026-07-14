#ifndef SOUL_UI_IVIEW_H
#define SOUL_UI_IVIEW_H

#include <QWidget>

namespace sc {

/**
 * @class IView
 * @brief UI 视图接口定义
 *
 * IView 定义了视图的标准接口，所有视图类都应实现此接口。
 * 支持显示/隐藏/关闭、标题设置、尺寸和位置管理。
 *
 * @see BaseView, Page, Window
 */
class IView {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IView() = default;

    /**
     * @brief 获取底层 QWidget
     * @return QWidget 指针
     */
    virtual QWidget* widget() = 0;

    /**
     * @brief 显示视图
     */
    virtual void show() = 0;

    /**
     * @brief 隐藏视图
     */
    virtual void hide() = 0;

    /**
     * @brief 关闭视图
     */
    virtual void close() = 0;

    /**
     * @brief 设置视图标题
     * @param title 标题文本
     */
    virtual void setTitle(const QString& title) = 0;

    /**
     * @brief 获取视图标题
     * @return 标题文本
     */
    virtual QString title() const = 0;

    /**
     * @brief 设置视图尺寸
     * @param width 宽度
     * @param height 高度
     */
    virtual void setSize(int width, int height) = 0;

    /**
     * @brief 获取视图尺寸
     * @return 尺寸
     */
    virtual QSize size() const = 0;

    /**
     * @brief 设置视图位置
     * @param x X坐标
     * @param y Y坐标
     */
    virtual void setPosition(int x, int y) = 0;

    /**
     * @brief 获取视图位置
     * @return 位置
     */
    virtual QPoint position() const = 0;
};

}

#endif
