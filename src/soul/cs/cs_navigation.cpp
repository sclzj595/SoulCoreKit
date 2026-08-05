// ============================================================================
// cs_navigation.cpp — CS 页面导航辅助实现 [v2.5.0]
// ============================================================================

#include "soul/cs/cs_navigation.h"
#include "soul/cs/cs_router.h"
#include "soul/application/application_context.h"

namespace sc::cs {

CsNavigation::CsNavigation(QObject* parent)
    : QObject(parent)
{
}

void CsNavigation::redirect(const QString& path, const QVariantMap& params) {
    // 对标 Spring 的 redirect: — 清空页面栈后跳转，原子操作避免闪烁
    ApplicationContext::instance().router().navigateRedirect(path, params);
}

void CsNavigation::forward(const QString& path, const QVariantMap& params) {
    // 对标 Spring 的 forward: — 保留页面栈，推入新页面
    ApplicationContext::instance().router().navigate(path, params);
}

void CsNavigation::back() {
    ApplicationContext::instance().router().navigateBack();
}

CsRequest CsNavigation::buildRequest(const QString& path, const QVariantMap& params) const {
    CsRequest request;
    request.path = path;
    request.body = params;

    // 分离查询参数（复用 CsRouter::parseQueryParams 避免重复逻辑）
    int queryIndex = path.indexOf('?');
    if (queryIndex >= 0) {
        request.path = path.left(queryIndex);
        QString queryString = path.mid(queryIndex + 1);
        request.queryParams = CsRouter::parseQueryParams(queryString);
    }

    return request;
}

} // namespace sc::cs