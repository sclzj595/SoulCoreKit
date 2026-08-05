#ifndef SOUL_CS_CONTROLLER_H
#define SOUL_CS_CONTROLLER_H

// ============================================================================
// cs_controller.h — CS 控制器基类 [v2.5.0]
// ============================================================================
//
// 对标 SpringBoot 的 @RestController + @RequestMapping。
// 继承 sc::ui::Page，复用页面生命周期，通过 Signal/Slot 实现 MVC 模式。
//
// 关系: CsController 继承 sc::Page，封装路由注册和请求处理。
// ============================================================================

#include <QObject>
#include <QWidget>
#include <QString>
#include <QMap>
#include <QHash>
#include <QVariantMap>
#include <QRegularExpression>
#include <functional>

#include "soul/cs/cs_global.h"
#include "soul/cs/cs_request.h"
#include "soul/cs/cs_error.h"
#include "soul/ui/page.h"

namespace sc::cs {

/// @brief 路由模式预编译缓存
///
/// 在 route() 注册时预编译正则表达式，避免 dispatch() 中重复编译。
/// 对标 Spring 的 RequestMappingInfo 的 Pattern 缓存。
struct RoutePatternInfo {
    QString handlerName;           // 字符串方式注册的 handler 名
    QRegularExpression compiledRegex;  // 预编译正则（仅含 {param} 模式时有效）
    bool hasParams = false;        // 是否包含路径参数
};

/// @brief CS 控制器基类（对标 Spring 的 @RestController）
///
/// 继承 sc::Page，复用 onEnter/onLeave/onBack 生命周期。
/// 通过 route() 注册路由映射，由 CsRouter 分发请求。
///
/// @par 设计决策: CsController 继承 QWidget 的原因
///
/// 在 CS 桌面架构中，Controller 同时承担 Page 角色：
/// - **路由分发**: 通过 route() 注册 handler，由 CsRouter 匹配路径后调用 dispatch()
/// - **页面生命周期**: 作为 QWidget，管理 onEnter/onLeave/onBack 等页面进入/离开事件
/// - **页面渲染**: 作为 QWidget 容器，承载子 Widget 的布局和显示
///
/// 这与 Web MVC 的 Controller（纯 Java 类）不同。在桌面应用中，
/// 路由分发和页面生命周期是天然耦合的——导航到 "user/list" 意味着
/// 同时创建 UserController 实例并将其推入页面栈。
///
/// 分层职责（在 CS 上下文中）:
/// ```
/// QWidget (View)  → 子 Widget 负责纯 UI 渲染
/// CsController    → 路由分发 + 页面生命周期 + 作为 View 容器
/// CsViewModel     → UI State 管理，不依赖 Service
/// CsService       → 业务逻辑，不依赖 UI
/// ```
///
/// 未来 v3.0 考虑: 提取 `ICsRouteHandler` 纯虚接口，分离路由分发与 QWidget 生命周期。
/// 当前阶段保持统一以简化 CS 架构的初始实现。
///
/// @par 使用示例
/// @code
/// class UserController : public CsController {
///     Q_OBJECT
/// public:
///     UserController() : CsController("user") {
///         route("list", "listUsers");
///         route("{id}", "userDetail");
///     }
///
/// public slots:
///     void listUsers(const CsRequest& req) {
///         auto users = m_service->findAll();
///         emit dataReady(QVariant::fromValue(users));
///     }
///     void userDetail(const CsRequest& req) {
///         int id = req.pathParam("id").toInt();
///         emit dataReady(QVariant::fromValue(m_service->findById(id)));
///     }
/// };
/// @endcode
class CsController : public sc::Page {
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param basePath 基础路径，如 "user"、"music"
    /// @param parent 父窗口
    explicit CsController(const QString& basePath, QWidget* parent = nullptr);

    ~CsController() override = default;

    /// @brief 注册路由（字符串方式）
    /// @param pattern 路由模式，如 "list"、"{id}"
    /// @param handlerName 处理函数名（Q_INVOKABLE 或 slot 方法名）
    ///
    /// 在注册时预编译 {param} 模式的正则表达式，避免 dispatch() 中重复编译。
    void route(const QString& pattern, const QString& handlerName);

    /// @brief 注册路由（成员函数指针方式，推荐）
    /// @tparam T 派生 Controller 类型
    /// @param pattern 路由模式，如 "list"、"{id}"
    /// @param handler 成员函数指针，签名为 void(const CsRequest&)
    ///
    /// 对标 Spring 的 @RequestMapping 注解，提供编译期类型安全。
    /// 使用成员函数指针替代字符串，避免运行时方法名查找失败。
    ///
    /// @par 使用示例
    /// @code
    /// class UserController : public CsController {
    ///     Q_OBJECT
    /// public:
    ///     UserController() : CsController("user") {
    ///         route("list", &UserController::listUsers);
    ///         route("{id}", &UserController::userDetail);
    ///     }
    ///
    /// public slots:
    ///     void listUsers(const CsRequest& req) { ... }
    ///     void userDetail(const CsRequest& req) { ... }
    /// };
    /// @endcode
    template<typename T>
    void route(const QString& pattern, void (T::*handler)(const CsRequest&)) {
        auto* derived = static_cast<T*>(this);
        m_handlers[pattern] = [derived, handler](const CsRequest& req) {
            (derived->*handler)(req);
        };
        // 同时注册到路由表，handlerName 为空表示函数指针方式
        // 预编译 {param} 模式的正则表达式
        RoutePatternInfo info;
        info.handlerName = QString();
        info.hasParams = pattern.contains('{') && pattern.contains('}');
        if (info.hasParams) {
            QString regexPattern = QRegularExpression::escape(pattern);
            regexPattern.replace("\\{", "(?<");
            regexPattern.replace("\\}", ">[^/]+)");
            info.compiledRegex = QRegularExpression("^" + regexPattern + "$");
        }
        m_routeInfos.insert(pattern, info);
    }

    /// @brief 获取基础路径
    QString basePath() const { return m_basePath; }

    /// @brief 获取所有已注册路由（pattern → handlerName，兼容旧接口）
    QMap<QString, QString> routes() const;

    /// @brief 获取所有路由模式信息（含预编译正则）
    const QMap<QString, RoutePatternInfo>& routeInfos() const { return m_routeInfos; }

    /// @brief 处理请求（由 CsRouter 调用）
    /// @param request 请求对象
    ///
    /// 默认实现通过 QMetaObject::invokeMethod 调用注册的 handler。
    /// 子类可重写以实现自定义路由逻辑。
    virtual void handleRequest(const CsRequest& request);

    /// @brief 解析 CsRequest 并调用对应 handler
    /// @param request 请求对象
    /// @return true 成功找到并调用 handler
    bool dispatch(const CsRequest& request);

signals:
    /// @brief 数据就绪信号（对标 ResponseEntity<T>）
    void dataReady(const QVariant& data);

    /// @brief 错误发生信号
    void errorOccurred(const CsError& error);

    /// @brief 页面导航请求信号（对标 redirect:/forward:）
    void navigationRequested(const QString& page, const QVariantMap& params);

protected:
    QString m_basePath;
    QMap<QString, RoutePatternInfo> m_routeInfos;  // pattern → RoutePatternInfo（含预编译正则）
    QHash<QString, std::function<void(const CsRequest&)>> m_handlers;  // pattern → handler (函数指针方式)
};

} // namespace sc::cs

#endif // SOUL_CS_CONTROLLER_H