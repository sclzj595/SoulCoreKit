#ifndef SOUL_ORM_BASE_REPOSITORY_H
#define SOUL_ORM_BASE_REPOSITORY_H

#include <QString>
#include <vector>
#include <memory>
#include <functional>
#include "soul/core/result.h"
#include "soul/data/repository.h"
#include "soul/orm/entity.h"
#include "soul/orm/query_wrapper.h"

namespace sc {
namespace orm {

template<typename T>
class BaseRepository : public data::IRepository<T, QString> {
public:
    Result<T> findById(const QString& id) override = 0;
    Result<std::vector<T>> findAll() override = 0;
    Result<T> save(const T& entity) override = 0;
    Result<void> removeById(const QString& id) override = 0;

    virtual Result<std::vector<T>> find(const QueryWrapper& query) = 0;
    virtual Result<void> remove(const QueryWrapper& query) = 0;
    virtual Result<int> count() override = 0;
    virtual Result<int> count(const QueryWrapper& query) = 0;

    virtual bool executeSql(const QString& sql, const std::vector<QVariant>& params = {}) = 0;

    virtual Result<T> findOne(const QueryWrapper& query) {
        QueryWrapper q = query;
        q.limit(1);
        auto result = find(q);
        if (!result.isOk()) {
            return result.unwrapErr();
        }
        auto list = result.unwrap();
        if (list.empty()) {
            return Error(ErrorCode::NotFound, "No record found");
        }
        return list[0];
    }
};

} // namespace orm
} // namespace sc

#endif