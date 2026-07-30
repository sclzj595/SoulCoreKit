#ifndef SOUL_DATA_TRANSACTION_H
#define SOUL_DATA_TRANSACTION_H

// ============================================================================
// transaction.h — 事务抽象接口与声明式事务支持
// ============================================================================
//
// 设计目标: 对标 SpringBoot @Transactional,提供声明式事务管理,降低
// 业务代码与事务管理代码的耦合。
//
// 核心组件:
//   - ITransaction:         事务抽象接口(commit/rollback/execute)
//   - ITransactionManager:  事务管理器接口(beginTransaction/withTransaction)
//   - TransactionScope:     RAII 事务作用域,析构时自动 commit 或 rollback
//   - withTransaction<T>:   声明式事务模板方法,自动管理事务边界
//
// 用法:
//   // 方式一: TransactionScope RAII
//   {
//       TransactionScope tx(txManager);
//       repo1.save(a);
//       repo2.save(b);
//       tx.commit();  // 显式提交,析构时若未提交则自动 rollback
//   }
//
//   // 方式二: withTransaction 声明式
//   auto result = txManager.withTransaction<int>([&]() -> Result<int> {
//       repo1.save(a);
//       repo2.save(b);
//       return 42;
//   });

#include <functional>
#include <memory>
#include <string>
#include "soul/core/result.h"
#include "soul/core/error.h"

namespace sc {
namespace data {

// ============================================================================
// ITransaction — 事务抽象接口
// ============================================================================
class ITransaction {
public:
    virtual ~ITransaction() = default;

    /// 提交事务
    virtual Result<void> commit() = 0;

    /// 回滚事务
    virtual Result<void> rollback() = 0;

    /// 在事务中执行操作
    virtual Result<void> execute(std::function<Result<void>()> operation) = 0;

    /// 检查事务是否活跃
    virtual bool isActive() const = 0;
};

// ============================================================================
// TransactionScope — RAII 事务作用域 [v1.9.1 新增]
// ============================================================================
//
// 自动管理事务生命周期。析构时若未提交则自动 rollback。
// 使用方式:
//   {
//       TransactionScope tx(txManager);
//       // ... 业务操作 ...
//       tx.commit();  // 可选:显式提交
//   }  // 析构时若未 commit 则自动 rollback
//
// @thread_safety 非线程安全,应在单线程内使用
class TransactionScope {
public:
    /// @brief 从 TransactionManager 创建事务作用域
    /// @param txManager 事务管理器
    explicit TransactionScope(class ITransactionManager& txManager);

    /// @brief 从已有事务创建事务作用域
    /// @param tx 已有事务
    explicit TransactionScope(std::unique_ptr<ITransaction> tx);

    ~TransactionScope();

    TransactionScope(const TransactionScope&) = delete;
    TransactionScope& operator=(const TransactionScope&) = delete;
    TransactionScope(TransactionScope&&) = delete;
    TransactionScope& operator=(TransactionScope&&) = delete;

    /// @brief 提交事务并标记为已提交(析构时不再 rollback)
    Result<void> commit();

    /// @brief 回滚事务并标记为已处理
    Result<void> rollback();

    /// @return 事务是否活跃
    bool isActive() const;

    /// @return 内部事务引用
    ITransaction* transaction() const noexcept { return m_tx.get(); }

private:
    std::unique_ptr<ITransaction> m_tx;
    bool m_committed = false;
    bool m_rolledBack = false;
};

// ============================================================================
// ITransactionManager — 事务管理器接口
// ============================================================================
class ITransactionManager {
public:
    virtual ~ITransactionManager() = default;

    /// 创建新事务
    virtual std::unique_ptr<ITransaction> beginTransaction() = 0;

    // ============================================================================
    // withTransaction<T> — 声明式事务模板方法 [v1.9.1 新增]
    // ============================================================================
    //
    // 对标 SpringBoot @Transactional,自动管理事务边界:
    //   - 操作成功: 自动 commit
    //   - 操作失败(Result 为 error)或异常: 自动 rollback
    //
    // 用法:
    //   auto result = txManager.withTransaction<int>([&]() -> Result<int> {
    //       repo.save(entity);
    //       return entity.id();
    //   });
    //
    // @tparam T 返回值类型(可为 void)
    // @param operation 业务操作(返回 Result<T>)
    // @return Result<T> 操作结果
    template<typename T>
    Result<T> withTransaction(std::function<Result<T>()> operation) {
        auto tx = beginTransaction();
        if (!tx) {
            return Result<T>::err(Error(ErrorCode::InternalError, "Failed to begin transaction"));
        }

        try {
            auto result = operation();
            if (result.isErr()) {
                tx->rollback();
                return result;
            }
            auto commitResult = tx->commit();
            if (commitResult.isErr()) {
                return Result<T>::err(Error(ErrorCode::InternalError,
                    "Transaction commit failed: " + commitResult.unwrapErr().message().toStdString()));
            }
            return result;
        } catch (const std::exception& e) {
            tx->rollback();
            return Result<T>::err(Error(ErrorCode::InternalError,
                std::string("Transaction failed: ") + e.what()));
        } catch (...) {
            tx->rollback();
            return Result<T>::err(Error(ErrorCode::InternalError, "Transaction failed: unknown exception"));
        }
    }

    /// @brief withTransaction<void> 特化
    Result<void> withTransactionVoid(std::function<Result<void>()> operation) {
        return withTransaction<void>(std::move(operation));
    }
};

} // namespace data
} // namespace sc

#endif // SOUL_DATA_TRANSACTION_H