// ============================================================================
// cs_error_handler.cpp — CS 全局错误处理器实现 [v2.1.0]
// ============================================================================

#include "soul/cs/cs_error_handler.h"
#include <QDebug>

namespace sc::cs {

CsErrorHandler::CsErrorHandler()
    : QObject(nullptr)
    , m_defaultHandler(nullptr)
{
}

CsErrorHandler& CsErrorHandler::instance() {
    static CsErrorHandler handler;
    return handler;
}

void CsErrorHandler::registerHandler(int errorCode, ErrorHandlerFunc handler) {
    if (handler) {
        m_handlers.insert(errorCode, std::move(handler));
    }
}

void CsErrorHandler::registerHandler(CsErrorCode errorCode, ErrorHandlerFunc handler) {
    registerHandler(static_cast<int>(errorCode), std::move(handler));
}

void CsErrorHandler::registerDefaultHandler(ErrorHandlerFunc handler) {
    m_defaultHandler = std::move(handler);
}

bool CsErrorHandler::handleError(const CsError& error) {
    if (error.isOk()) return true;

    // 1. 精确匹配
    auto it = m_handlers.find(error.code());
    if (it != m_handlers.end() && it.value()) {
        bool handled = it.value()(error);
        if (handled) {
            emit errorHandled(error);
            return true;
        }
    }

    // 2. 默认处理
    if (m_defaultHandler) {
        bool handled = m_defaultHandler(error);
        if (handled) {
            emit errorHandled(error);
            return true;
        }
    }

    // 3. 未处理
    qWarning() << "CsErrorHandler: unhandled error -" << error.toString();
    emit errorUnhandled(error);
    return false;
}

bool CsErrorHandler::handleError(const sc::Error& error) {
    return handleError(CsError::from(error));
}

void CsErrorHandler::clear() {
    m_handlers.clear();
    m_defaultHandler = nullptr;
}

} // namespace sc::cs
