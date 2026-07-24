#ifndef SOUL_DATA_MEMORY_REPOSITORY_H
#define SOUL_DATA_MEMORY_REPOSITORY_H

#include <vector>
#include <algorithm>
#include <map>
#include "soul/data/repository.h"

namespace sc {
namespace data {

/**
 * @class MemoryRepository
 * @brief 内存数据仓库实现
 *
 * MemoryRepository 是 IRepository 的通用内存实现。
 * 数据存储在内存中，进程退出后丢失。适用于临时数据存储或测试场景。
 *
 * 使用方式：
 * @code
 * MemoryRepository<User, QString> repo(
 *     [](const User& u) { return u.id; }
 * );
 * repo.save(user);
 * auto result = repo.findById("123");
 * @endcode
 *
 * @tparam T  实体类型
 * @tparam Id 实体ID类型，默认为 QString
 */
template<typename T, typename Id = QString>
class MemoryRepository : public IRepository<T, Id> {
public:
    /// ID 提取函数类型
    using IdExtractor = std::function<Id(const T&)>;

    /**
     * @brief 构造函数
     * @param extractor 从实体中提取 ID 的函数
     */
    explicit MemoryRepository(IdExtractor extractor)
        : m_idExtractor(std::move(extractor)) {}

    Result<T> findById(const Id& id) override {
        auto it = std::find_if(m_data.begin(), m_data.end(),
            [this, &id](const T& e) { return m_idExtractor(e) == id; });
        if (it != m_data.end()) {
            return *it;
        }
        return Error(ErrorCode::NotFound, "Entity not found");
    }

    Result<std::vector<T>> findAll() override {
        return m_data;
    }

    Result<T> save(const T& entity) override {
        Id id = m_idExtractor(entity);
        auto it = std::find_if(m_data.begin(), m_data.end(),
            [this, &id](const T& e) { return m_idExtractor(e) == id; });
        if (it != m_data.end()) {
            *it = entity;
        } else {
            m_data.push_back(entity);
        }
        return entity;
    }

    Result<void> removeById(const Id& id) override {
        auto it = std::remove_if(m_data.begin(), m_data.end(),
            [this, &id](const T& e) { return m_idExtractor(e) == id; });
        if (it != m_data.end()) {
            m_data.erase(it, m_data.end());
            return {};
        }
        return Error(ErrorCode::NotFound, "Entity not found");
    }

private:
    IdExtractor m_idExtractor;
    std::vector<T> m_data;
};

}
}

#endif // SOUL_DATA_MEMORY_REPOSITORY_H
