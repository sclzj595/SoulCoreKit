// ============================================================================
// cs_controller.cpp — CS 控制器基类实现 [v2.5.0]
// ============================================================================

#include "soul/cs/cs_controller.h"
#include <QMetaObject>
#include <QMetaMethod>
#include <QRegularExpression>

namespace sc::cs {

CsController::CsController(const QString& basePath, QWidget* parent)
    : sc::Page(parent)
    , m_basePath(basePath)
{
    setPageTitle(basePath);
}

void CsController::route(const QString& pattern, const QString& handlerName) {
    // 预编译 {param} 模式的正则表达式，避免 dispatch() 中重复编译
    RoutePatternInfo info;
    info.handlerName = handlerName;
    info.hasParams = pattern.contains('{') && pattern.contains('}');
    if (info.hasParams) {
        QString regexPattern = QRegularExpression::escape(pattern);
        regexPattern.replace("\\{", "(?<");
        regexPattern.replace("\\}", ">[^/]+)");
        info.compiledRegex = QRegularExpression("^" + regexPattern + "$");
    }
    m_routeInfos.insert(pattern, info);
}

QMap<QString, QString> CsController::routes() const {
    QMap<QString, QString> result;
    for (auto it = m_routeInfos.constBegin(); it != m_routeInfos.constEnd(); ++it) {
        result.insert(it.key(), it.value().handlerName);
    }
    return result;
}

void CsController::handleRequest(const CsRequest& request) {
    dispatch(request);
}

bool CsController::dispatch(const CsRequest& request) {
    // 如果 CsRouter::match() 已经确定了 handlerName（对标 Spring 的 HandlerMethod），
    // 优先使用函数指针方式（编译期类型安全）。
    if (!request.handlerName.isEmpty()) {
        auto handlerIt = m_handlers.find(request.handlerName);
        if (handlerIt != m_handlers.end() && handlerIt.value()) {
            handlerIt.value()(request);
            return true;
        }

        // [v2.5.1] DEPRECATED: 字符串方式 QMetaObject::invokeMethod 回退。
        // 运行时方法名查找无编译期检查，计划 v3.0 移除。
        // 迁移指南: 将 handler 注册从 registerRoute(name, handlerName) 迁移到
        // registerRoute(name, &Controller::handlerMethod)。
        QByteArray handlerNameUtf8 = request.handlerName.toUtf8();
        bool invoked = QMetaObject::invokeMethod(this,
                                  handlerNameUtf8.constData(),
                                  Qt::AutoConnection,
                                  Q_ARG(CsRequest, request));
        if (!invoked) {
            emit errorOccurred(CsError(CsErrorCode::InternalError,
                                       QString("Handler '%1' not found or signature mismatch in controller '%2'")
                                           .arg(request.handlerName, m_basePath)));
        }
        return invoked;
    }

    // 回退：遍历路由表查找匹配（兼容直接调用 dispatch() 而不经过 CsRouter 的场景）
    // 从请求中提取子路径（去掉 basePath 前缀）
    QString subPath = request.path;
    if (subPath.startsWith(m_basePath + "/")) {
        subPath = subPath.mid(m_basePath.length() + 1);
    } else if (subPath == m_basePath) {
        subPath.clear();
    }

    // 遍历路由表查找匹配
    for (auto it = m_routeInfos.constBegin(); it != m_routeInfos.constEnd(); ++it) {
        const QString& pattern = it.key();
        const RoutePatternInfo& info = it.value();

        // 简单匹配：直接比较
        // "index" 路由仅匹配空 subPath（即 Controller 根路径）
        if (pattern == subPath || 
            (pattern.isEmpty() && subPath.isEmpty()) ||
            (pattern == "index" && subPath.isEmpty())) {
            // 优先使用函数指针方式
            auto handlerIt = m_handlers.find(pattern);
            if (handlerIt != m_handlers.end() && handlerIt.value()) {
                handlerIt.value()(request);
                return true;
            }
            // [v2.5.1] DEPRECATED: 字符串回退（同上方说明）
            if (!info.handlerName.isEmpty()) {
                QByteArray handlerNameUtf8 = info.handlerName.toUtf8();
                bool invoked = QMetaObject::invokeMethod(this, 
                                          handlerNameUtf8.constData(),
                                          Qt::AutoConnection,
                                          Q_ARG(CsRequest, request));
                if (!invoked) {
                    emit errorOccurred(CsError(CsErrorCode::InternalError,
                                               QString("Handler '%1' not found in controller '%2'")
                                                   .arg(info.handlerName, m_basePath)));
                }
                return invoked;
            }
        }

        // 路径参数匹配：使用预编译正则（避免每次 dispatch 重复编译）
        if (info.hasParams) {
            if (info.compiledRegex.match(subPath).hasMatch()) {
                auto handlerIt = m_handlers.find(pattern);
                if (handlerIt != m_handlers.end() && handlerIt.value()) {
                    handlerIt.value()(request);
                    return true;
                }
                // [v2.5.1] DEPRECATED: 字符串回退（同上方说明）
                if (!info.handlerName.isEmpty()) {
                    QByteArray handlerNameUtf8 = info.handlerName.toUtf8();
                    bool invoked = QMetaObject::invokeMethod(this,
                                              handlerNameUtf8.constData(),
                                              Qt::AutoConnection,
                                              Q_ARG(CsRequest, request));
                    if (!invoked) {
                        emit errorOccurred(CsError(CsErrorCode::InternalError,
                                                   QString("Handler '%1' not found in controller '%2'")
                                                       .arg(info.handlerName, m_basePath)));
                    }
                    return invoked;
                }
            }
        }
    }

    // 未找到匹配路由
    emit errorOccurred(CsError(CsErrorCode::RouteNotFound,
                               QString("No route matched for '%1' in controller '%2'")
                                   .arg(subPath, m_basePath)));
    return false;
}

} // namespace sc::cs