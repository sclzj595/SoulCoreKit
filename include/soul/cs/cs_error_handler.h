#ifndef SOUL_CS_ERROR_HANDLER_H
#define SOUL_CS_ERROR_HANDLER_H

// ============================================================================
// cs_error_handler.h — CS 全局错误处理 [v2.5.0]
// ============================================================================
//
// 对标 Spring 的 @ControllerAdvice + @ExceptionHandler。
// 提供全局错误捕获和统一错误响应机制。
//
// 关系: 独立模块，可与 CsRouter 集成以拦截所有 Controller 的错误。
// ============================================================================

#include <QObject>
#include <QHash>
#include <QString>
#include <functional>
#include <memory>

#include "soul/cs/cs_global.h"
#include "soul/cs/cs_error.h"

namespace sc::cs {

/// @brief 错误处理回调类型
/// @param error 错误对象
/// @return true 表示错误已处理，false 表示需要继续传播
using ErrorHandlerFunc = std::function<bool(const CsError& error)>;

/// @brief CS 全局错误处理器（对标 Spring 的 @ControllerAdvice）
///
/// 支持按错误码注册处理函数，未匹配的错误使用默认处理函数。
/// 单例模式，全局共享。
///
/// @par 使用示例
/// @code
/// auto& handler = CsErrorHandler::instance();
/// handler.registerHandler(CsErrorCode::ValidationFailed, [](const CsError& error) {
///     CsDialogManager::show("toast", error.message());
///     return true;
/// });
/// handler.registerDefaultHandler([](const CsError& error) {
///     qWarning() << "Unhandled error:" << error.toString();
///     return false;
/// });
/// @endcode
class CsErrorHandler : public QObject {
    Q_OBJECT

public:
    /// @brief 获取单例
    static CsErrorHandler& instance();

    /// @brief 注册错误处理函数
    /// @param errorCode 错误码
    /// @param handler 处理函数
    void registerHandler(int errorCode, ErrorHandlerFunc handler);

    /// @brief 注册 CsErrorCode 处理函数
    void registerHandler(CsErrorCode errorCode, ErrorHandlerFunc handler);

    /// @brief 注册默认处理函数（未匹配到特定处理函数时调用）
    /// @param handler 处理函数
    void registerDefaultHandler(ErrorHandlerFunc handler);

    /// @brief 处理错误
    /// @param error 错误对象
    /// @return true 错误已被处理
    ///
    /// 按以下顺序查找处理函数:
    /// 1. 精确匹配错误码的处理函数
    /// 2. 默认处理函数
    /// 3. 返回 false（未处理）
    bool handleError(const CsError& error);

    /// @brief 处理 sc::Error
    bool handleError(const sc::Error& error);

    /// @brief 清除所有处理函数
    void clear();

signals:
    /// @brief 错误处理完成信号
    void errorHandled(const CsError& error);

    /// @brief 未处理的错误信号
    void errorUnhandled(const CsError& error);

private:
    CsErrorHandler();
    ~CsErrorHandler() override = default;
    CsErrorHandler(const CsErrorHandler&) = delete;
    CsErrorHandler& operator=(const CsErrorHandler&) = delete;

    QHash<int, ErrorHandlerFunc> m_handlers;  // errorCode → handler
    ErrorHandlerFunc m_defaultHandler;
};

} // namespace sc::cs

#endif // SOUL_CS_ERROR_HANDLER_H