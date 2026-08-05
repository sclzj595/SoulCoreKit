#ifndef SOUL_CS_DIALOG_MANAGER_H
#define SOUL_CS_DIALOG_MANAGER_H

// ============================================================================
// cs_dialog_manager.h — CS 对话框管理器 [v2.5.0]
// ============================================================================
//
// 对标 Spring MVC 的 Modal/Alert 处理。
// 包装 sc::ui::Dialog / sc::ui::BaseDialog，提供统一对话框管理。
//
// 关系: 包装 sc::ui::Dialog 和 sc::ui::Toast，不替代现有 UI 组件。
// ============================================================================

#include <QObject>
#include <QString>
#include <QWidget>
#include <functional>

#include "soul/cs/cs_global.h"

namespace sc::cs {

/// @brief 对话框类型
enum class DialogType {
    Confirm,     ///< 确认对话框（是/否）
    Alert,       ///< 提示对话框（确定）
    Toast,       ///< Toast 消息（自动消失）
    Input,       ///< 输入对话框
};

/// @brief 对话框选项
struct DialogOptions {
    QString title;                ///< 对话框标题
    QString message;              ///< 消息内容
    QString confirmText = "确定";  ///< 确认按钮文本
    QString cancelText = "取消";   ///< 取消按钮文本
    int toastDuration = 3000;     ///< Toast 持续时间（毫秒）
    QWidget* parent = nullptr;    ///< 父窗口
};

/// @brief CS 对话框管理器（对标 Spring 的 Modal/Alert）
///
/// 单例模式，提供统一的对话框创建和管理。
/// 支持确认框、提示框、Toast 和输入框。
///
/// @par 使用示例
/// @code
/// auto& dlg = CsDialogManager::instance();
///
/// // 确认对话框
/// dlg.confirm("确定删除?", [](bool confirmed) {
///     if (confirmed) { /* 执行删除 */ }
/// });
///
/// // Toast 消息
/// dlg.toast("操作成功", 3000);
///
/// // 输入对话框
/// dlg.input("请输入名称", [](const QString& text) {
///     if (!text.isEmpty()) { /* 使用输入 */ }
/// });
/// @endcode
class CsDialogManager : public QObject {
    Q_OBJECT

public:
    /// @brief 获取单例
    static CsDialogManager& instance();

    /// @brief 显示确认对话框
    /// @param message 消息内容
    /// @param callback 回调函数（true=确认，false=取消）
    /// @param options 对话框选项
    void confirm(const QString& message,
                 std::function<void(bool)> callback = nullptr,
                 const DialogOptions& options = {});

    /// @brief 显示提示对话框
    /// @param message 消息内容
    /// @param callback 回调函数
    /// @param options 对话框选项
    void alert(const QString& message,
               std::function<void()> callback = nullptr,
               const DialogOptions& options = {});

    /// @brief 显示 Toast 消息
    /// @param message 消息内容
    /// @param duration 持续时间（毫秒），0 表示使用默认值
    /// @param options 对话框选项
    void toast(const QString& message,
               int duration = 0,
               const DialogOptions& options = {});

    /// @brief 显示输入对话框
    /// @param message 提示消息
    /// @param callback 回调函数（参数为用户输入文本）
    /// @param options 对话框选项
    void input(const QString& message,
               std::function<void(const QString&)> callback = nullptr,
               const DialogOptions& options = {});

signals:
    /// @brief 对话框已显示信号
    void dialogShown(DialogType type, const QString& message);

    /// @brief 对话框已关闭信号
    void dialogClosed(DialogType type);

private:
    CsDialogManager();
    ~CsDialogManager() override = default;
    CsDialogManager(const CsDialogManager&) = delete;
    CsDialogManager& operator=(const CsDialogManager&) = delete;
};

} // namespace sc::cs

#endif // SOUL_CS_DIALOG_MANAGER_H