#ifndef SOUL_DATA_REPOSITORY_H
#define SOUL_DATA_REPOSITORY_H

#include <vector>
#include <optional>
#include <QList>
#include "soul/core/result.h"

namespace sc {
namespace data {

/**
 * @class IRepository
 * @brief 统一数据仓库抽象接口
 *
 * IRepository 是整个 SoulCoreKit 数据访问层的唯一 Repository 抽象。
 * 它定义了标准的 CRUD 业务能力，不包含任何数据库/SQL/存储介质细节。
 *
 * 设计原则：
 * - Repository 只关心"如何访问对象"，不关心"如何存"
 * - 所有实现（SQLite、JSON、Memory、Remote 等）都实现同一个接口
 * - ID 类型可参数化，默认为 QString
 *
 * 使用方式：
 * @code
 * class UserRepository : public IRepository<User, QString> {
 *     // 实现 findById, findAll, save, removeById, existsById
 * };
 *
 * IRepository<User, QString>* repo = new SQLiteRepository<User>("data.db");
 * auto result = repo->findById("user-123");
 * @endcode
 *
 * @tparam T  实体类型
 * @tparam Id 实体ID类型，默认为 QString
 */
template<typename T, typename Id = QString>
class IRepository {
public:
    virtual ~IRepository() = default;

    virtual Result<T> findById(const Id& id) = 0;
    virtual Result<std::vector<T>> findAll() = 0;
    virtual Result<T> save(const T& entity) = 0;
    virtual Result<void> removeById(const Id& id) = 0;

    virtual Result<bool> existsById(const Id& id) {
        auto result = findById(id);
        if (result.isOk()) return true;
        if (result.unwrapErr().code() == ErrorCode::NotFound) return false;
        return result.unwrapErr();
    }

    virtual Result<std::vector<T>> saveBatch(const std::vector<T>& entities) {
        std::vector<T> saved;
        saved.reserve(entities.size());
        for (const auto& entity : entities) {
            auto result = save(entity);
            if (!result.isOk()) {
                return result.unwrapErr();
            }
            saved.push_back(result.unwrap());
        }
        return saved;
    }

    virtual Result<void> removeBatch(const std::vector<Id>& ids) {
        for (const auto& id : ids) {
            auto result = removeById(id);
            if (!result.isOk()) {
                return result;
            }
        }
        return {};
    }

    virtual Result<int> count() {
        auto result = findAll();
        if (!result.isOk()) {
            return result.unwrapErr();
        }
        return static_cast<int>(result.unwrap().size());
    }
};

}
}

#endif // SOUL_DATA_REPOSITORY_H
