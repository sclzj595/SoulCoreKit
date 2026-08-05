#ifndef SOUL_CS_NAVIGATION_H
#define SOUL_CS_NAVIGATION_H

// ============================================================================
// cs_navigation.h — CS 页面导航辅助 [v2.5.0]
// ============================================================================
//
// 对标 Spring MVC 的 Redirect/Forward。
// 包装 sc::ui::Navigation，提供基于路由的导航辅助方法。
//
// 关系: 包装 sc::ui::Navigation，与 CsRouter 协同工作。
// ============================================================================

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "soul/cs/cs_global.h"
#include "soul/cs/cs_request.h"

namespace sc::cs {

/// @brief CS 页面导航辅助类（对标 Spring 的 Redirect/Forward）
///
/// 提供便捷的导航方法，与 CsRouter 协同使用。
/// 对标 Spring MVC 的 ModelAndView("redirect:/path") 和 ModelAndView("forward:/path")。
///
/// @par 使用示例
/// @code
/// // 在 Controller 中通过实例调用
/// CsNavigation nav;
/// void UserController::createUser(const CsRequest& req) {
///     auto result = m_service->create(user);
///     if (result.isOk()) {
///         nav.redirect("user/list");
///     } else {
///         nav.forward("user/create", {{"error", result.unwrapErr().message()}});
///     }
/// }
/// @endcode
class CsNavigation : public QObject {
    Q_OBJECT

public:
    /// @brief 构造函数
    explicit CsNavigation(QObject* parent = nullptr);
    ~CsNavigation() override = default;

    /// @brief 重定向导航（对标 Spring 的 redirect:）
    /// @param path 目标路径
    /// @param params 参数
    ///
    /// 清空当前页面栈，跳转到新路径。
    /// 对标 Spring MVC 的 "redirect:/path"。
    void redirect(const QString& path, const QVariantMap& params = {});

    /// @brief 转发导航（对标 Spring 的 forward:）
    /// @param path 目标路径
    /// @param params 参数
    ///
    /// 保留当前页面栈，推入新路径。
    /// 对标 Spring MVC 的 "forward:/path"。
    void forward(const QString& path, const QVariantMap& params = {});

    /// @brief 返回上一页
    void back();

    /// @brief 构建导航请求
    /// @param path 路径
    /// @param params 参数
    /// @return CsRequest 对象
    CsRequest buildRequest(const QString& path, const QVariantMap& params = {}) const;
};

} // namespace sc::cs

#endif // SOUL_CS_NAVIGATION_H