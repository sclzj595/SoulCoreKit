#ifndef SOUL_CS_ERROR_H
#define SOUL_CS_ERROR_H

// ============================================================================
// cs_error.h — CS 错误类型 [v2.5.0]
// ============================================================================
//
// 对标 Spring 的 ResponseEntity + HttpStatus 错误状态映射。
// 与 sc::Error 兼容，增加 CS 场景的错误分类。
// ============================================================================

#include <QString>
#include "soul/core/error.h"

namespace sc::cs {

/// @brief CS 场景错误码扩展
enum class CsErrorCode {
    Ok = 0,
    NavigationFailed = 1001,    ///< 页面导航失败
    RouteNotFound = 1002,       ///< 路由未找到
    ValidationFailed = 1003,    ///< 表单校验失败
    Unauthorized = 1004,        ///< 未授权
    Forbidden = 1005,           ///< 禁止访问
    ServiceUnavailable = 1006,  ///< 服务不可用
    DialogCancelled = 1007,     ///< 对话框取消
    WindowCreateFailed = 1008,  ///< 窗口创建失败
    InternalError = 1009,       ///< 内部错误
};

/// @brief CS 错误包装类
///
/// 包装 sc::ErrorCode 或 CsErrorCode，提供统一的错误处理接口。
/// 对标 Spring 的 ResponseEntity.status(HttpStatus.BAD_REQUEST)
class CsError {
public:
    CsError() : m_code(static_cast<int>(CsErrorCode::Ok)), m_message() {}

    /// @brief 从 CsErrorCode 构造
    explicit CsError(CsErrorCode code, const QString& message = {})
        : m_code(static_cast<int>(code)), m_message(message) {}

    /// @brief 从 sc::Error 转换
    explicit CsError(const sc::Error& error)
        : m_code(static_cast<int>(error.code()))
        , m_message(error.message()) {}

    /// @brief 从 sc::Error 创建 CsError
    static CsError from(const sc::Error& error) {
        return CsError(error);
    }

    int code() const { return m_code; }
    const QString& message() const { return m_message; }
    bool isOk() const { return m_code == 0; }

    QString toString() const {
        return QString("[CS-%1] %2").arg(m_code).arg(m_message);
    }

private:
    int m_code;
    QString m_message;
};

} // namespace sc::cs

#endif // SOUL_CS_ERROR_H