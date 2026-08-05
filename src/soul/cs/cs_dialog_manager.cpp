// ============================================================================
// cs_dialog_manager.cpp — CS 对话框管理器实现 [v2.1.0]
// ============================================================================

#include "soul/cs/cs_dialog_manager.h"
#include "soul/ui/dialog.h"
#include "soul/ui/toast.h"
#include <QInputDialog>
#include <QLineEdit>
#include <QTimer>

namespace sc::cs {

CsDialogManager::CsDialogManager()
    : QObject(nullptr)
{
}

CsDialogManager& CsDialogManager::instance() {
    static CsDialogManager manager;
    return manager;
}

void CsDialogManager::confirm(const QString& message,
                               std::function<void(bool)> callback,
                               const DialogOptions& options) {
    QString title = options.title.isEmpty() ? QString::fromUtf8("确认") : options.title;
    int result = sc::Dialog::confirm(options.parent, title, message);

    bool confirmed = (result == 1);  // QDialog::Accepted or Yes
    emit dialogShown(DialogType::Confirm, message);

    if (callback) {
        callback(confirmed);
    }

    emit dialogClosed(DialogType::Confirm);
}

void CsDialogManager::alert(const QString& message,
                             std::function<void()> callback,
                             const DialogOptions& options) {
    QString title = options.title.isEmpty() ? QString::fromUtf8("提示") : options.title;
    sc::Dialog::info(options.parent, title, message);

    emit dialogShown(DialogType::Alert, message);

    if (callback) {
        callback();
    }

    emit dialogClosed(DialogType::Alert);
}

void CsDialogManager::toast(const QString& message,
                             int duration,
                             const DialogOptions& options) {
    int actualDuration = duration > 0 ? duration : options.toastDuration;
    sc::Toast::info(message, actualDuration);

    emit dialogShown(DialogType::Toast, message);
    // Toast 自动关闭，由 Toast 内部 Timer 处理
    // 对标 Spring 的 FlashAttribute — 在 duration 后发射 dialogClosed
    QTimer::singleShot(actualDuration, this, [this]() {
        emit dialogClosed(DialogType::Toast);
    });
}

void CsDialogManager::input(const QString& message,
                             std::function<void(const QString&)> callback,
                             const DialogOptions& options) {
    QString title = options.title.isEmpty() ? QString::fromUtf8("输入") : options.title;

    emit dialogShown(DialogType::Input, message);

    // 对标 Spring 的 @Valid + 表单输入 — 使用 Qt 内置 QInputDialog
    bool ok = false;
    QString text = QInputDialog::getText(options.parent, title, message,
                                         QLineEdit::Normal, QString(), &ok);

    if (callback) {
        callback(ok ? text : QString());
    }

    emit dialogClosed(DialogType::Input);
}

} // namespace sc::cs