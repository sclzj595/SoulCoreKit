#ifndef SOUL_UI_LOADING_H
#define SOUL_UI_LOADING_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

namespace sc {

/**
 * @class Loading
 * @brief 加载指示器组件
 *
 * Loading 提供加载状态的可视化指示，支持：
 * - 文本提示
 * - 进度条显示
 * - 确定/不确定模式
 * - 全局加载指示器
 *
 * 使用方式：
 * @code
 * Loading::showGlobal("加载中...");
 * // 执行耗时操作
 * Loading::hideGlobal();
 * @endcode
 */
class Loading : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit Loading(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Loading() override = default;

    /**
     * @brief 设置加载文本
     * @param text 文本内容
     */
    void setText(const QString& text);

    /**
     * @brief 获取加载文本
     * @return 文本内容
     */
    QString text() const;

    /**
     * @brief 显示/隐藏进度条
     * @param show 是否显示
     */
    void showProgress(bool show);

    /**
     * @brief 设置进度值
     * @param value 进度值（0-100）
     */
    void setProgress(int value);

    /**
     * @brief 获取进度值
     * @return 进度值
     */
    int progress() const;

    /**
     * @brief 设置为不确定模式
     * @param indeterminate 是否为不确定模式
     */
    void setIndeterminate(bool indeterminate);

    /**
     * @brief 显示全局加载指示器
     * @param text 加载文本
     */
    static void showGlobal(const QString& text = "Loading...");

    /**
     * @brief 隐藏全局加载指示器
     */
    static void hideGlobal();

    /**
     * @brief 更新全局加载进度
     * @param value 进度值（0-100）
     */
    static void updateGlobalProgress(int value);

private:
    QLabel* m_textLabel;
    QProgressBar* m_progressBar;
    QVBoxLayout* m_layout;
};

}

#endif
