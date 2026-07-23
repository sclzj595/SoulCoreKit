#ifndef SOUL_UI_BASE_VIEW_H
#define SOUL_UI_BASE_VIEW_H

#include "i_view.h"

namespace sc {

/**
 * @class BaseView
 * @brief UI 视图基类，实现 IView 接口
 *
 * BaseView 是所有视图类的基类，提供了 IView 接口的默认实现。
 * 继承自 QWidget 和 IView，支持视图初始化和布局设置。
 *
 * 使用方式：
 * @code
 * class MyView : public BaseView {
 * protected:
 *     void initUI() override {
 *         // 初始化 UI 组件
 *     }
 *     void setupLayout() override {
 *         // 设置布局
 *     }
 * };
 * @endcode
 *
 * @see IView, Page, Window
 */
class BaseView : public QWidget, public IView {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit BaseView(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~BaseView() override = default;

    /**
     * @brief 获取底层 QWidget
     * @return QWidget 指针
     */
    QWidget* widget() override;

    /**
     * @brief 显示视图
     */
    void show() override;

    /**
     * @brief 隐藏视图
     */
    void hide() override;

    /**
     * @brief 关闭视图
     */
    void close() override;

    /**
     * @brief 设置视图标题
     * @param title 标题文本
     */
    void setTitle(const QString& title) override;

    /**
     * @brief 获取视图标题
     * @return 标题文本
     */
    QString title() const override;

    /**
     * @brief 设置视图尺寸
     * @param width 宽度
     * @param height 高度
     */
    void setSize(int width, int height) override;

    /**
     * @brief 获取视图尺寸
     * @return 尺寸
     */
    QSize size() const override;

    /**
     * @brief 设置视图位置
     * @param x X坐标
     * @param y Y坐标
     */
    void setPosition(int x, int y) override;

    /**
     * @brief 获取视图位置
     * @return 位置
     */
    QPoint position() const override;

protected:
    /**
     * @brief 初始化 UI 组件（虚函数，子类可重写）
     */
    virtual void initUI();

    /**
     * @brief 设置布局（虚函数，子类可重写）
     */
    virtual void setupLayout();
};

}

#endif
