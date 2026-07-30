// ============================================================================
// transaction.cpp — 事务抽象接口实现
// ============================================================================

#include "soul/data/transaction.h"
#include "soul/core/error.h"

namespace sc {
namespace data {

// ============================================================================
// TransactionScope 实现
// ============================================================================

TransactionScope::TransactionScope(ITransactionManager& txManager)
    : m_tx(txManager.beginTransaction()) {
}

TransactionScope::TransactionScope(std::unique_ptr<ITransaction> tx)
    : m_tx(std::move(tx)) {
}

TransactionScope::~TransactionScope() {
    if (m_tx && !m_committed && !m_rolledBack) {
        m_tx->rollback();
    }
}

Result<void> TransactionScope::commit() {
    if (!m_tx) {
        return Result<void>::err(Error(ErrorCode::InternalError, "No active transaction"));
    }
    if (m_committed) {
        return Result<void>::err(Error(ErrorCode::InternalError, "Transaction already committed"));
    }
    if (m_rolledBack) {
        return Result<void>::err(Error(ErrorCode::InternalError, "Transaction already rolled back"));
    }

    auto result = m_tx->commit();
    if (result.isOk()) {
        m_committed = true;
    }
    return result;
}

Result<void> TransactionScope::rollback() {
    if (!m_tx) {
        return Result<void>::err(Error(ErrorCode::InternalError, "No active transaction"));
    }
    if (m_committed) {
        return Result<void>::err(Error(ErrorCode::InternalError, "Transaction already committed"));
    }
    if (m_rolledBack) {
        return Result<void>::err(Error(ErrorCode::InternalError, "Transaction already rolled back"));
    }

    auto result = m_tx->rollback();
    if (result.isOk()) {
        m_rolledBack = true;
    }
    return result;
}

bool TransactionScope::isActive() const {
    return m_tx != nullptr && !m_committed && !m_rolledBack;
}

} // namespace data
} // namespace sc