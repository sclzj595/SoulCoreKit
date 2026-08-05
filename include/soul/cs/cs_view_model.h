#ifndef SOUL_CS_VIEW_MODEL_H
#define SOUL_CS_VIEW_MODEL_H

// ============================================================================
// cs_view_model.h — CS 视图模型基类 [v2.5.0]
// ============================================================================
//
// 对标 SpringBoot 的 ModelAndView + ViewResolver。
// 扩展 sc::ui::BaseViewModel，增加 Result<T> 类型化和 Service 注入。
//
// 关系: CsViewModel 继承 sc::ui::BaseViewModel，复用属性管理和变更通知。
// ============================================================================

#include <QObject>
#include <QPointer>
#include <QRunnable>
#include <QString>
#include <QThreadPool>
#include <QVariant>

#include "soul/cs/cs_global.h"
#include "soul/cs/cs_error.h"
#include "soul/ui/base_view_model.h"

namespace sc::cs {

// 前向声明（AsyncTask 需要 CsViewModel 指针）
class CsViewModel;

/// @brief CS 视图模型基类（对标 Spring 的 ModelAndView）
///
/// 继承 sc::ui::BaseViewModel，增加:
/// - Result<T> 类型化支持
/// - CsError 错误处理
/// - 加载状态管理
/// - Service 注入支持
///
/// @par 使用示例
/// @code
/// // ✅ 正确: ViewModel 通过 Controller 获取数据（架构文档推荐）
/// class UserListViewModel : public CsViewModel {
///     Q_OBJECT
/// public:
///     UserListViewModel(UserController* controller)
///         : CsViewModel("UserList"), m_controller(controller) {
///         // 连接 Controller 的 dataReady 信号
///         QObject::connect(m_controller, &CsController::dataReady,
///                          this, &UserListViewModel::onUsersLoaded);
///     }
///
///     void load() {
///         setLoading(true);
///         m_controller->handleRequest(CsRequest("list"));
///     }
///
/// public slots:
///     void onUsersLoaded(const QVariant& data) {
///         setValue("users", data);
///         setLoading(false);
///     }
///
/// private:
///     UserController* m_controller;  // 由 DI 注入
/// };
///
/// // ⚠️ 简化示例: ViewModel 直接持有 Service（仅用于原型/演示）
/// class UserListViewModelSimple : public CsViewModel {
///     Q_OBJECT
/// public:
///     UserListViewModelSimple(std::shared_ptr<UserService> service)
///         : CsViewModel("UserList"), m_service(std::move(service)) {}
///
///     void load() {
///         setLoading(true);
///         auto result = m_service->findAll();
///         if (result.isOk()) {
///             setValue("users", toVariantList(result.unwrap()));
///             setLoading(false);
///         } else {
///             setError(result.unwrapErr());
///         }
///     }
/// };
/// @endcode
class CsViewModel : public sc::ui::BaseViewModel {
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param viewModelName ViewModel 名称（用于调试）
    /// @param parent 父对象
    explicit CsViewModel(const QString& viewModelName, QObject* parent = nullptr);

    ~CsViewModel() override = default;

    /// @brief 获取 ViewModel 名称
    QString viewModelName() const { return m_viewModelName; }

    /// @brief 设置加载状态（重写 BaseViewModel，增加 CsError 联动）
    /// @param loading 是否正在加载
    void setLoading(bool loading);

    /// @brief 获取加载状态
    bool isLoading() const;

    /// @brief 设置错误状态
    /// @param error CsError 对象
    void setError(const CsError& error);

    /// @brief 清除错误状态
    void clearError();

    /// @brief 获取当前错误
    CsError currentError() const { return m_currentError; }

    /// @brief 是否有错误
    bool hasError() const { return !m_currentError.isOk(); }

    /// @brief 执行异步操作（对标 Spring 的 @Async）
    /// @tparam Func 可调用对象类型（返回 Result<T> 或可被 onSuccess 接收的类型）
    /// @tparam SuccessFunc 成功回调类型
    /// @tparam ErrorFunc 失败回调类型
    /// @param asyncFunc 异步操作函数（在 QThreadPool 工作线程执行）
    /// @param onSuccess 成功回调（在主线程执行）
    /// @param onError 失败回调（在主线程执行，接收 QString 错误消息）
    ///
    /// 自动设置 loading 状态。
    /// asyncFunc 在工作线程执行，完成后通过 Qt::QueuedConnection 回到主线程。
    /// 使用 QPointer 守卫，确保 ViewModel 销毁后不会访问悬垂指针。
    ///
    /// 实现定义在类外（AsyncTask 之后），因为 AsyncTask 需要 CsViewModel 完整类型。
    template<typename Func, typename SuccessFunc, typename ErrorFunc>
    void executeAsync(Func&& asyncFunc, SuccessFunc&& onSuccess, ErrorFunc&& onError);

signals:
    /// @brief 错误状态变更信号
    void errorChanged(const QString& errorMessage);

    /// @brief 数据就绪信号
    void dataReady(const QVariant& data);

protected:
    QString m_viewModelName;
    CsError m_currentError;
    bool m_isLoading = false;
};

// ============================================================================
// AsyncTask — executeAsync 的 QRunnable 辅助类
// ============================================================================
///
/// 对标 Spring 的 @Async 注解 + TaskExecutor。
/// 将 asyncFunc 投递到 QThreadPool，完成后通过 QMetaObject::invokeMethod
/// 回到主线程调用 onSuccess/onError 回调。
///
/// 使用 QPointer<CsViewModel> 防止 ViewModel 在异步任务执行期间被销毁。
///
/// 必须在 CsViewModel 完整定义之后定义，因为 run() 内调用 vm->setLoading()。
template<typename Func, typename SuccessFunc, typename ErrorFunc>
struct AsyncTask : public QRunnable {
    Func asyncFunc;
    SuccessFunc onSuccess;
    ErrorFunc onError;
    QPointer<CsViewModel> vm;

    AsyncTask(Func&& f, SuccessFunc&& s, ErrorFunc&& e, CsViewModel* v)
        : asyncFunc(std::forward<Func>(f))
        , onSuccess(std::forward<SuccessFunc>(s))
        , onError(std::forward<ErrorFunc>(e))
        , vm(v)
    {
        // 自动删除，对标 Spring 的 TaskExecutor 的 fire-and-forget 语义
        setAutoDelete(true);
    }

    void run() override {
        try {
            auto result = asyncFunc();
            if (vm) {
                QMetaObject::invokeMethod(vm, [this, result = std::move(result)]() mutable {
                    if (!vm) return;
                    vm->setLoading(false);
                    onSuccess(std::move(result));
                }, Qt::QueuedConnection);
            }
        } catch (const std::exception& e) {
            if (vm) {
                QString errorMsg = QString::fromUtf8(e.what());
                QMetaObject::invokeMethod(vm, [this, errorMsg]() {
                    if (!vm) return;
                    vm->setLoading(false);
                    onError(errorMsg);
                }, Qt::QueuedConnection);
            }
        } catch (...) {
            if (vm) {
                QMetaObject::invokeMethod(vm, [this]() {
                    if (!vm) return;
                    vm->setLoading(false);
                    onError(QStringLiteral("Unknown async error"));
                }, Qt::QueuedConnection);
            }
        }
    }
};

// ============================================================================
// CsViewModel::executeAsync 模板实现
// ============================================================================
///
/// 必须在 AsyncTask 完整定义之后，因为创建 AsyncTask 实例需要完整类型。
template<typename Func, typename SuccessFunc, typename ErrorFunc>
void CsViewModel::executeAsync(Func&& asyncFunc, SuccessFunc&& onSuccess, ErrorFunc&& onError) {
    setLoading(true);

    auto* task = new AsyncTask<Func, SuccessFunc, ErrorFunc>(
        std::forward<Func>(asyncFunc),
        std::forward<SuccessFunc>(onSuccess),
        std::forward<ErrorFunc>(onError),
        this
    );
    QThreadPool::globalInstance()->start(task);
}

} // namespace sc::cs

#endif // SOUL_CS_VIEW_MODEL_H