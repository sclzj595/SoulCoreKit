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
    virtual Result<T> findById(const QString& id) override {
        QueryWrapper query;
        query.eq("id", id);
        return findOne(query);
    }

    virtual Result<std::vector<T>> findAll() override {
        return find(QueryWrapper());
    }

    virtual Result<T> save(const T& entity) = 0;
    virtual Result<void> removeById(const QString& id) override {
        QueryWrapper query;
        query.eq("id", id);
        return remove(query);
    }

    virtual Result<std::vector<T>> find(const QueryWrapper& query) = 0;
    virtual Result<void> remove(const QueryWrapper& query) = 0;

    virtual Result<int> count() override {
        return count(QueryWrapper());
    }
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

    virtual Result<bool> existsById(const QString& id) override {
        auto result = findById(id);
        if (result.isOk()) return true;
        if (result.unwrapErr().code() == ErrorCode::NotFound) return false;
        return result.unwrapErr();
    }

    virtual Result<std::vector<T>> saveBatch(const std::vector<T>& entities) override {
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
};

} // namespace orm
} // namespace sc

#endif