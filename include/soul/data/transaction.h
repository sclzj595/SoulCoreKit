#ifndef SOUL_DATA_TRANSACTION_H
#define SOUL_DATA_TRANSACTION_H

#include <functional>
#include "soul/core/result.h"

namespace sc {
namespace data {

/**
 * @class ITransaction
 * @brief 事务抽象接口
 *
 * ITransaction 定义了跨数据源的事务操作接口。
 * 不同实现（SQL事务、内存事务、分布式事务等）实现同一接口。
 *
 * 使用方式：
 * @code
 * auto tx = dataSource.beginTransaction();
 * tx->execute([&]() -> Result<void> {
 *     repo1.save(a);
 *     repo2.save(b);
 *     return {};
 * });
 * tx->commit();
 * @endcode
 */
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

/**
 * @class ITransactionManager
 * @brief 事务管理器接口
 *
 * 负责创建和管理事务生命周期。
 */
class ITransactionManager {
public:
    virtual ~ITransactionManager() = default;

    /// 创建新事务
    virtual std::unique_ptr<ITransaction> beginTransaction() = 0;
};

}
}

#endif // SOUL_DATA_TRANSACTION_H
