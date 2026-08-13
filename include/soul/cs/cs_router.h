#ifndef SOUL_CS_ROUTER_H
#define SOUL_CS_ROUTER_H

// ============================================================================
// cs_router.h — CS 页面/窗口导航路由器 [v2.5.0]
// ============================================================================
//
// 对标 SpringBoot 的 DispatcherServlet + RequestMappingHandlerMapping。
// 包装 sc::ui::Navigation，增加路由表注册和路径参数解析。
//
// 关系: CsRouter 包装 sc::Navigation，CsController 通过 CsRouter 分发请求。
// ============================================================================

#include <QObject>
#include <QHash>
#include <QList>
#include <QString>
#include <QStackedWidget>
#include <QVariantMap>
#include <QRegularExpression>
#include <functional>

#include "soul/cs/cs_global.h"
#include "soul/cs/cs_request.h"

namespace sc {
class ControllerRegistry;  // friend declaration in CsRouter
}

namespace sc::cs {

class CsController;
class CsModule;
class CsErrorHandler;

/// @brief 路由条目
struct RouteEntry {
    CsController* controller = nullptr;
    QString handlerName;
    QString fullPattern;  // 完整模式: "user/{id}"

    /// @brief 预编译的路径匹配正则（惰性初始化）
    ///
    /// 在 registerController 时编译，避免每次 match() 重复编译。
    /// 仅当 pattern 包含 {param} 时有效。
    QRegularExpression compiledRegex;

    /// @brief 路径参数名列表（按捕获组顺序）
    ///
    /// 在 registerController 时提取，避免每次 parsePathParams 重复解析。
    QStringList paramNames;

    /// @brief 是否为静态路由（无路径参数）
    bool isStatic() const { return paramNames.isEmpty(); }
};

/// @brief CS 页面路由器（对标 Spring 的 DispatcherServlet）
///
/// 由 ApplicationContext 创建并持有唯一实例，不自行管理单例。
/// 支持路径参数解析（如 user/{id} → pathParams["id"]=123）。
///
/// @par 使用示例
/// @code
/// auto& router = ApplicationContext::instance().router();
/// // Controller 通过 ControllerRegistry 统一注册，无需直接调用 registerController
/// router.setRootWidget(stackedWidget);
/// router.navigate("user/123");
/// @endcode
class CsRouter : public QObject {
    Q_OBJECT

    // ControllerRegistry 需要通过 registerController 注册路由
    // 对标 Spring 的 AbstractHandlerMethodMapping 内部调用
    friend class sc::ControllerRegistry;

public:
    /// @brief 构造函数（由 ApplicationContext 调用）
    /// @param errorHandler CsErrorHandler 引用，用于全局错误处理
    explicit CsRouter(CsErrorHandler& errorHandler);
    ~CsRouter();

    /// @brief 导航到指定路径（push 到页面栈）
    /// @param path 目标路径，如 "user/123" 或 "user/list?sort=name"
    /// @param params 额外参数
    ///
    /// 解析路径，匹配路由表，创建 CsRequest 并调用 Controller::dispatch()。
    /// 同时调用 sc::Navigation::push() 将 Controller (Page) 推入页面栈。
    void navigate(const QString& path, const QVariantMap& params = {});

    /// @brief 前进导航（对标 Spring 的 forward:）
    /// @param path 目标路径
    /// @param params 额外参数
    ///
    /// 保留当前页面栈，推入新页面。与 navigate() 等效。
    void navigateForward(const QString& path, const QVariantMap& params = {});

    /// @brief 返回上一页
    void navigateBack();

    /// @brief 替换当前页面
    /// @param path 目标路径
    /// @param params 额外参数
    void navigateReplace(const QString& path, const QVariantMap& params = {});

    /// @brief 返回根页面
    void navigateToRoot();

    /// @brief 重定向导航（对标 Spring 的 redirect:）
    /// @param path 目标路径
    /// @param params 额外参数
    ///
    /// 清空页面栈并跳转到新路径，原子操作避免中间状态闪烁。
    void navigateRedirect(const QString& path, const QVariantMap& params = {});

    /// @brief 设置根导航组件
    /// @param stack QStackedWidget 指针
    void setRootWidget(QStackedWidget* stack);

    /// @brief 获取当前页面
    QWidget* currentWidget() const;

    /// @brief 获取页面栈数量
    int stackCount() const;

    /// @brief 列出所有已注册路由（对标 /actuator/mappings）
    QList<RouteEntry> routes() const;

    /// @brief 通过路径匹配路由
    /// @param path 请求路径
    /// @return 匹配的路由条目，未匹配返回空条目
    RouteEntry match(const QString& path) const;

    /// @brief 解析查询参数
    /// @param path 带查询参数的路径，如 "user/list?sort=name&page=1"
    /// @return 解析出的查询参数
    static QVariantMap parseQueryParams(const QString& path);

    /// @brief 注册 Controller 的所有路由
    /// @param controller Controller 实例
    ///
    /// 对标 Spring 的 AbstractHandlerMethodMapping.registerHandler()。
    /// 外部代码应通过 ControllerRegistry::registerAllRoutes() 间接调用。
    void registerController(CsController* controller);

private:
    CsRouter(const CsRouter&) = delete;
    CsRouter& operator=(const CsRouter&) = delete;

    /// @brief 连接 Controller 信号到 CsRouter（对标 Spring 的 HandlerAdapter）
    ///
    /// 连接 CsController::navigationRequested → CsRouter::navigate
    /// 连接 CsController::errorOccurred → CsErrorHandler::handleError
    /// @param controller Controller 实例
    void connectControllerSignals(CsController* controller);

    QHash<QString, RouteEntry> m_routeTable;  // "user/{id}" → RouteEntry
    QList<CsController*> m_controllers;
    CsErrorHandler& m_errorHandler;  ///< 全局错误处理器引用（由 ApplicationContext 注入）
};

} // namespace sc::cs

#endif // SOUL_CS_ROUTER_H