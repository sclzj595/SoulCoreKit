#ifndef SOUL_UI_PAGE_H
#define SOUL_UI_PAGE_H

#include <QWidget>
#include <QString>
#include "soul/base/base_widget.h"

namespace sc {

/**
 * @class Page
 * @brief 页面组件基类
 *
 * Page 是页面级别的组件基类，提供页面生命周期管理：
 * - onEnter()：页面进入时调用
 * - onLeave()：页面离开时调用
 * - onBack()：返回操作时调用
 * - 页面标题和副标题管理
 *
 * 使用方式：
 * @code
 * class HomePage : public Page {
 * public:
 *     void onEnter() override {
 *         // 页面进入逻辑
 *     }
 *     void onLeave() override {
 *         // 页面离开逻辑
 *     }
 * };
 * @endcode
 *
 * @see BaseWidget, Navigation
 */
class Page : public BaseWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit Page(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Page() override = default;

    /**
     * @brief 设置页面标题
     * @param title 标题文本
     */
    void setPageTitle(const QString& title);

    /**
     * @brief 获取页面标题
     * @return 标题文本
     */
    QString pageTitle() const;

    /**
     * @brief 设置页面副标题
     * @param subtitle 副标题文本
     */
    void setPageSubtitle(const QString& subtitle);

    /**
     * @brief 获取页面副标题
     * @return 副标题文本
     */
    QString pageSubtitle() const;

    /**
     * @brief 页面进入回调（虚函数，子类可重写）
     */
    virtual void onEnter();

    /**
     * @brief 页面离开回调（虚函数，子类可重写）
     */
    virtual void onLeave();

    /**
     * @brief 返回操作回调（虚函数，子类可重写）
     */
    virtual void onBack();

signals:
    /**
     * @brief 页面进入信号
     */
    void pageEnter();

    /**
     * @brief 页面离开信号
     */
    void pageLeave();

    /**
     * @brief 返回按钮按下信号
     */
    void backPressed();

protected:
    /**
     * @brief 设置页面内容
     * @param content 内容组件
     */
    void setupContent(QWidget* content);

private:
    QString m_title;
    QString m_subtitle;
};

}

#endif
