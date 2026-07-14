#ifndef SOUL_UI_DIALOG_H
#define SOUL_UI_DIALOG_H

#include <QDialog>
#include <QString>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <functional>

namespace sc {

/**
 * @enum DialogType
 * @brief 对话框类型枚举
 */
enum class DialogType {
    Info,     ///< 信息对话框
    Warning,  ///< 警告对话框
    Error,    ///< 错误对话框
    Success,  ///< 成功对话框
    Confirm   ///< 确认对话框
};

/**
 * @enum DialogButton
 * @brief 对话框按钮类型枚举
 */
enum class DialogButton {
    Ok,      ///< 确定按钮
    Cancel,  ///< 取消按钮
    Yes,     ///< 是按钮
    No       ///< 否按钮
};

/**
 * @class Dialog
 * @brief 对话框组件
 *
 * Dialog 提供统一的对话框样式和交互，支持多种类型：
 * - Info：信息提示
 * - Warning：警告提示
 * - Error：错误提示
 * - Success：成功提示
 * - Confirm：确认对话框
 *
 * 使用方式：
 * @code
 * Dialog::info(parent, "提示", "操作成功");
 * if (Dialog::confirm(parent, "确认", "确定删除？")) {
 *     // 执行删除
 * }
 * @endcode
 *
 * @see DialogType, DialogButton
 */
class Dialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit Dialog(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Dialog() override = default;

    /**
     * @brief 设置对话框类型
     * @param type 对话框类型
     */
    void setType(DialogType type);

    /**
     * @brief 设置对话框标题
     * @param title 标题文本
     */
    void setTitle(const QString& title);

    /**
     * @brief 设置对话框消息
     * @param message 消息文本
     */
    void setMessage(const QString& message);

    /**
     * @brief 添加按钮
     * @param button 按钮类型
     * @param text 按钮文本（可选）
     */
    void addButton(DialogButton button, const QString& text = "");

    /**
     * @brief 设置按钮点击回调
     * @param callback 回调函数
     */
    void setOnButtonClicked(std::function<void(DialogButton)> callback);

    /**
     * @brief 显示信息对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息
     * @return 用户点击的按钮
     */
    static int info(QWidget* parent, const QString& title, const QString& message);

    /**
     * @brief 显示警告对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息
     * @return 用户点击的按钮
     */
    static int warning(QWidget* parent, const QString& title, const QString& message);

    /**
     * @brief 显示错误对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息
     * @return 用户点击的按钮
     */
    static int error(QWidget* parent, const QString& title, const QString& message);

    /**
     * @brief 显示成功对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息
     * @return 用户点击的按钮
     */
    static int success(QWidget* parent, const QString& title, const QString& message);

    /**
     * @brief 显示确认对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息
     * @return 用户点击的按钮（Yes/No）
     */
    static int confirm(QWidget* parent, const QString& title, const QString& message);

private:
    /**
     * @brief 初始化 UI
     */
    void setupUI();

    /**
     * @brief 获取图标路径
     * @param type 对话框类型
     * @return 图标路径
     */
    QString getIconPath(DialogType type) const;

    DialogType m_type;
    QString m_title;
    QString m_message;
    std::function<void(DialogButton)> m_callback;

    QVBoxLayout* m_mainLayout;
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
    QHBoxLayout* m_buttonLayout;
};

}

#endif
