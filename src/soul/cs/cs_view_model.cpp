// ============================================================================
// cs_view_model.cpp — CS 视图模型基类实现 [v2.1.0]
// ============================================================================

#include "soul/cs/cs_view_model.h"

namespace sc::cs {

CsViewModel::CsViewModel(const QString& viewModelName, QObject* parent)
    : sc::ui::BaseViewModel(parent)
    , m_viewModelName(viewModelName)
    , m_currentError()
    , m_isLoading(false)
{
}

bool CsViewModel::isLoading() const {
    return m_isLoading;
}

void CsViewModel::setLoading(bool loading) {
    if (m_isLoading == loading) return;
    m_isLoading = loading;
    // 通知 BaseViewModel 发射 loadingChanged 信号（对标 Spring 的 @RefreshScope 状态传播）
    BaseViewModel::setLoading(loading);
}

void CsViewModel::setError(const CsError& error) {
    m_currentError = error;
    emit errorChanged(error.message());
}

void CsViewModel::clearError() {
    m_currentError = CsError();
    emit errorChanged(QString());
}

} // namespace sc::cs
