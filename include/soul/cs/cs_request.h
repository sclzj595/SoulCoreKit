#ifndef SOUL_CS_REQUEST_H
#define SOUL_CS_REQUEST_H

// ============================================================================
// cs_request.h — CS 请求对象 [v2.5.0]
// ============================================================================
//
// 对标 Spring 的 HttpServletRequest + 路径参数解析。
// 封装了页面导航时传递的所有请求信息。
// ============================================================================

#include <QString>
#include <QVariantMap>
#include <QMetaType>

namespace sc::cs {

/// @brief CS 请求对象（对标 Spring 的 HttpServletRequest）
///
/// 当 CsRouter 导航到页面时，将路径参数、查询参数等封装为 CsRequest
/// 传递给 CsController 的 slot 处理函数。
///
/// @par 使用示例
/// @code
/// void UserController::userDetail(const CsRequest& req) {
///     int userId = req.pathParams["id"].toInt();  // 从 user/{id} 解析
///     QString sort = req.queryParams["sort"].toString();  // ?sort=name
/// }
/// @endcode
struct CsRequest {
    /// @brief 完整请求路径，如 "user/detail"
    QString path;

    /// @brief 匹配到的 handler 名称（由 CsRouter::match() 填充）
    ///
    /// 对标 Spring 的 HandlerMethod。
    /// CsController::dispatch() 优先使用此字段直接调用 handler，
    /// 避免重复遍历路由表。
    QString handlerName;

    /// @brief 路径参数: user/{id} → {"id": 123}
    QVariantMap pathParams;

    /// @brief 查询参数: ?sort=name&page=1 → {"sort": "name", "page": 1}
    QVariantMap queryParams;

    /// @brief 请求体（对标 @RequestBody）
    QVariantMap body;

    /// @brief 请求上下文（对标 SecurityContext / Session）
    QVariantMap context;

    /// @brief 检查是否为空请求
    bool isEmpty() const { return path.isEmpty(); }

    /// @brief 获取路径参数，不存在时返回默认值
    QVariant pathParam(const QString& key, const QVariant& defaultValue = {}) const {
        return pathParams.value(key, defaultValue);
    }

    /// @brief 获取查询参数，不存在时返回默认值
    QVariant queryParam(const QString& key, const QVariant& defaultValue = {}) const {
        return queryParams.value(key, defaultValue);
    }
};

} // namespace sc::cs

Q_DECLARE_METATYPE(sc::cs::CsRequest)

#endif // SOUL_CS_REQUEST_H