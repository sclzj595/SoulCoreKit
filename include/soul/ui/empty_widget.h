#ifndef SOUL_UI_EMPTY_WIDGET_H
#define SOUL_UI_EMPTY_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace sc {

/**
 * @class EmptyWidget
 * @brief 空状态组件
 *
 * EmptyWidget 用于显示列表为空时的提示，包含：
 * - 图标
 * - 标题
 * - 副标题
 * - 操作按钮
 *
 * 使用方式：
 * @code
 * EmptyWidget* empty = new EmptyWidget(this);
 * empty->setIcon(Icon::fromResource(":/icons/empty.png"));
 * empty->setTitle("暂无数据");
 * empty->setSubtitle("点击下方按钮添加");
 * @endcode
 */
class EmptyWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit EmptyWidget(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~EmptyWidget() override = default;

    /**
     * @brief 设置图标
     * @param icon 图标
     */
    void setIcon(const QIcon& icon);

    /**
     * @brief 设置标题
     * @param title 标题文本
     */
    void setTitle(const QString& title);

    /**
     * @brief 设置副标题
     * @param subtitle 副标题文本
     */
    void setSubtitle(const QString& subtitle);

    /**
     * @brief 设置按钮文本
     * @param text 按钮文本
     */
    void setButtonText(const QString& text);

    /**
     * @brief 显示/隐藏按钮
     * @param show 是否显示
     */
    void showButton(bool show);

signals:
    /**
     * @brief 按钮点击信号
     */
    void buttonClicked();

private:
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QPushButton* m_actionButton;
    QVBoxLayout* m_layout;
};

}

#endif
