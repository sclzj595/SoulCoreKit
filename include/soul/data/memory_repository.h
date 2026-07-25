#ifndef SOUL_DATA_MEMORY_REPOSITORY_H
#define SOUL_DATA_MEMORY_REPOSITORY_H

#include <vector>
#include <algorithm>
#include <map>
#include <mutex>
#include "soul/data/repository.h"

namespace sc {
namespace data {

template<typename T, typename Id = QString>
class MemoryRepository : public IRepository<T, Id> {
public:
    using IdExtractor = std::function<Id(const T&)>;

    explicit MemoryRepository(IdExtractor extractor)
        : m_idExtractor(std::move(extractor)) {}

    Result<T> findById(const Id& id) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find_if(m_data.begin(), m_data.end(),
            [this, &id](const T& e) { return m_idExtractor(e) == id; });
        if (it != m_data.end()) {
            return *it;
        }
        return Error(ErrorCode::NotFound, "Entity not found");
    }

    Result<std::vector<T>> findAll() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_data;
    }

    Result<T> save(const T& entity) override {
        std::lock_guard<std::mutex> lock(m_mutex);
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
        std::lock_guard<std::mutex> lock(m_mutex);
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
    mutable std::mutex m_mutex;
};

}
}

#endif // SOUL_DATA_MEMORY_REPOSITORY_H
