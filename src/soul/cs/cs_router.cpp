// ============================================================================
// cs_router.cpp — CS 页面路由器实现 [v2.5.0]
// ============================================================================

#include "soul/cs/cs_router.h"
#include "soul/cs/cs_controller.h"
#include "soul/cs/cs_error_handler.h"
#include "soul/cs/cs_module.h"
#include "soul/ui/navigation.h"
#include <QDebug>
#include <QRegularExpression>
#include <QUrlQuery>

namespace sc::cs {

CsRouter::CsRouter(CsErrorHandler& errorHandler)
    : QObject(nullptr)
    , m_errorHandler(errorHandler)
{
}

CsRouter::~CsRouter() = default;

void CsRouter::registerController(CsController* controller) {
    if (!controller) return;

    m_controllers.append(controller);

    // 连接 Controller 信号到 CsRouter 和 CsErrorHandler
    // 对标 Spring 的 HandlerAdapter 注册
    connectControllerSignals(controller);

    // 注册 Controller 的所有路由
    const auto& routes = controller->routes();
    for (auto it = routes.constBegin(); it != routes.constEnd(); ++it) {
        const QString& pattern = it.key();
        const QString& handlerName = it.value();

        QString fullPattern = controller->basePath();
        if (!pattern.isEmpty() && pattern != "index") {
            fullPattern += "/" + pattern;
        }

        RouteEntry entry;
        entry.controller = controller;
        // 函数指针方式: handlerName 为空时使用 pattern 作为 key，
        // 使得 CsController::dispatch 能直接通过 m_handlers[pattern] 查找
        entry.handlerName = handlerName.isEmpty() ? pattern : handlerName;
        entry.fullPattern = fullPattern;

        // 预编译路径参数正则（对标 Spring RequestMappingHandlerMapping 的 Pattern 缓存）
        if (pattern.contains('{') && pattern.contains('}')) {
            // 提取参数名
            int pos = 0;
            while (pos < pattern.length()) {
                int start = pattern.indexOf('{', pos);
                if (start < 0) break;
                int end = pattern.indexOf('}', start);
                if (end < 0) break;
                entry.paramNames.append(pattern.mid(start + 1, end - start - 1));
                pos = end + 1;
            }

            // 编译完整路径的正则
            QString regexPattern = QRegularExpression::escape(fullPattern);
            regexPattern.replace("\\{", "(");
            regexPattern.replace("\\}", ")");
            entry.compiledRegex = QRegularExpression("^" + regexPattern + "$");
        }

        // 检测路由冲突（对标 Spring 的 AmbiguousMappingException）
        if (m_routeTable.contains(fullPattern)) {
            qWarning() << "CsRouter: Duplicate route registration for pattern:"
                       << fullPattern << "— overwriting previous entry.";
        }

        m_routeTable.insert(fullPattern, entry);
    }
}

// ============================================================================
// navigateForward — 对标 Spring 的 forward:
// ============================================================================
void CsRouter::navigateForward(const QString& path, const QVariantMap& params) {
    // forward 语义: 保留页面栈，推入新页面（与 navigate 等效）
    navigate(path, params);
}

// ============================================================================
// connectControllerSignals — 对标 Spring 的 HandlerAdapter
// ============================================================================
void CsRouter::connectControllerSignals(CsController* controller) {
    if (!controller) return;

    // 连接 navigationRequested → CsRouter::navigate
    // 对标 Spring 的 redirect:/forward: 处理
    QObject::connect(controller, &CsController::navigationRequested,
                     this, [this](const QString& page, const QVariantMap& params) {
                         navigate(page, params);
                     });

    // 连接 errorOccurred → CsErrorHandler::handleError
    // 对标 Spring 的 @ControllerAdvice 全局异常处理
    QObject::connect(controller, &CsController::errorOccurred,
                     this, [this](const CsError& error) {
                         m_errorHandler.handleError(error);
                     });
}

// ============================================================================
// 路径参数解析（使用预编译正则，性能优化版本）
// ============================================================================
///
/// 对标 Spring 的 UriTemplate 变量提取。
/// 利用 RouteEntry 中预编译的 compiledRegex 和 paramNames，
/// 避免运行时重复编译正则表达式。
///
/// 必须在 navigate() 和 navigateReplace() 之前定义，确保编译期可见。
static QVariantMap parsePathParamsFromEntry(const QString& path, const RouteEntry& entry) {
    QVariantMap params;
    if (entry.isStatic()) return params;

    QRegularExpressionMatch match = entry.compiledRegex.match(path);
    if (match.hasMatch()) {
        for (int i = 0; i < entry.paramNames.size(); ++i) {
            params.insert(entry.paramNames[i], match.captured(i + 1));
        }
    }

    return params;
}

void CsRouter::navigate(const QString& path, const QVariantMap& params) {
    // 分离路径和查询参数
    QString cleanPath = path;
    QString queryString;
    int queryIndex = path.indexOf('?');
    if (queryIndex >= 0) {
        cleanPath = path.left(queryIndex);
        queryString = path.mid(queryIndex + 1);
    }

    // 匹配路由
    RouteEntry matched = match(cleanPath);
    if (!matched.controller) {
        // 路由未找到 — 通过 CsErrorHandler 统一处理
        // 对标 Spring 的 NoHandlerFoundException → @ControllerAdvice
        CsError routeError(CsErrorCode::RouteNotFound,
                           QString("No route found for path: '%1'").arg(cleanPath));
        m_errorHandler.handleError(routeError);

        // 尝试直接作为页面标题导航（兜底行为）
        sc::Navigation::push(nullptr, cleanPath);
        return;
    }

    // 构建 CsRequest
    CsRequest request;
    request.path = cleanPath;
    request.handlerName = matched.handlerName;
    request.pathParams = parsePathParamsFromEntry(cleanPath, matched);
    request.queryParams = parseQueryParams(queryString);
    request.body = params;

    // 分发请求到 Controller
    matched.controller->dispatch(request);

    // 导航到 Controller 页面
    sc::Navigation::push(matched.controller, matched.controller->pageTitle());
}

void CsRouter::navigateBack() {
    sc::Navigation::pop();
}

void CsRouter::navigateReplace(const QString& path, const QVariantMap& params) {
    // 分离路径和查询参数（对标 navigate() 的对称实现）
    QString cleanPath = path;
    QString queryString;
    int queryIndex = path.indexOf('?');
    if (queryIndex >= 0) {
        cleanPath = path.left(queryIndex);
        queryString = path.mid(queryIndex + 1);
    }

    RouteEntry matched = match(cleanPath);
    if (!matched.controller) {
        CsError routeError(CsErrorCode::RouteNotFound,
                           QString("No route found for path: '%1'").arg(cleanPath));
        m_errorHandler.handleError(routeError);
        sc::Navigation::replace(nullptr, cleanPath);
        return;
    }

    CsRequest request;
    request.path = cleanPath;
    request.handlerName = matched.handlerName;
    request.pathParams = parsePathParamsFromEntry(cleanPath, matched);
    request.queryParams = parseQueryParams(queryString);
    request.body = params;

    matched.controller->dispatch(request);
    sc::Navigation::replace(matched.controller, matched.controller->pageTitle());
}

void CsRouter::navigateToRoot() {
    sc::Navigation::popToRoot();
}

void CsRouter::navigateRedirect(const QString& path, const QVariantMap& params) {
    // 对标 Spring 的 redirect: — 清空栈后原子替换
    navigateToRoot();
    navigateReplace(path, params);
}

void CsRouter::setRootWidget(QStackedWidget* stack) {
    sc::Navigation::setRootWidget(stack);
}

QWidget* CsRouter::currentWidget() const {
    return sc::Navigation::currentWidget();
}

int CsRouter::stackCount() const {
    return sc::Navigation::stackCount();
}

QList<RouteEntry> CsRouter::routes() const {
    return m_routeTable.values();
}

RouteEntry CsRouter::match(const QString& path) const {
    // 1. 精确匹配
    if (m_routeTable.contains(path)) {
        return m_routeTable.value(path);
    }

    // 2. 模式匹配（路径参数）— 使用预编译正则
    for (auto it = m_routeTable.constBegin(); it != m_routeTable.constEnd(); ++it) {
        const RouteEntry& entry = it.value();
        if (entry.isStatic()) continue;

        QRegularExpressionMatch match = entry.compiledRegex.match(path);
        if (match.hasMatch()) {
            return entry;
        }
    }

    return RouteEntry{};
}

QVariantMap CsRouter::parseQueryParams(const QString& path) {
    QVariantMap params;
    if (path.isEmpty()) return params;

    QStringList pairs = path.split('&');
    for (const QString& pair : pairs) {
        int eqIndex = pair.indexOf('=');
        if (eqIndex > 0) {
            QString key = pair.left(eqIndex);
            QString value = pair.mid(eqIndex + 1);
            params.insert(key, value);
        }
    }

    return params;
}

} // namespace sc::cs