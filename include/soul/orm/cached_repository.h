#ifndef SOUL_ORM_CACHED_REPOSITORY_H
#define SOUL_ORM_CACHED_REPOSITORY_H

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include "soul/cache/icache.h"
#include "soul/core/result.h"
#include "soul/data/repository.h"

namespace sc {
namespace orm {

/**
 * @brief Repository 缓存装饰器
 *
 * 包装任意 IRepository 实现,为其读取操作添加缓存层,降低底层存储压力。
 * 采用装饰器模式,不修改被包装对象,可任意组合(如 SQLiteRepository + MemoryCache)。
 *
 * @par 缓存策略
 * - findById: 先查缓存,未命中查 delegate,命中后回填
 * - existsById: 通过 findById 间接利用缓存
 * - save: 转发到 delegate,成功后用 idExtractor 提取 ID 并更新缓存
 * - removeById: 转发到 delegate,成功后从缓存移除
 * - findAll/saveBatch/removeBatch/count: 直接转发(不缓存,避免一致性问题)
 *
 * @par ID 提取
 * 通过 idExtractor 函数从实体 T 中提取缓存键。
 * 若不提供(传 nullptr),save/remove 不会自动失效缓存,用户需手动管理。
 *
 * @tparam T   实体类型
 * @tparam Id  ID 类型,需与 IRepository 的 Id 一致
 *
 * @thread_safety Thread-Safe — 依赖底层 ICache 的线程安全保证
 *
 * @see data::IRepository, cache::ICache
 */
template<typename T, typename Id = QString>
class CachedRepository : public data::IRepository<T, Id> {
public:
    /**
     * @brief ID 提取器类型
     * @param entity 实体引用
     * @return 实体的 ID
     */
    using IdExtractor = std::function<Id(const T&)>;

    /**
     * @brief 构造函数
     * @param delegate 被装饰的 Repository(非空)
     * @param cache    缓存实例(非空)
     * @param idExtractor ID 提取器(可为 nullptr,此时 save 不自动更新缓存)
     * @param ttl      缓存 TTL(默认 5 分钟)
     * @throws std::invalid_argument 若 delegate 或 cache 为空
     */
    CachedRepository(std::shared_ptr<data::IRepository<T, Id>> delegate,
                     std::shared_ptr<cache::ICache<Id, T>> cache,
                     IdExtractor idExtractor = nullptr,
                     std::chrono::milliseconds ttl = std::chrono::minutes(5))
        : m_delegate(std::move(delegate))
        , m_cache(std::move(cache))
        , m_idExtractor(std::move(idExtractor))
        , m_ttl(ttl) {
        if (!m_delegate) {
            throw std::invalid_argument("CachedRepository: delegate must not be null");
        }
        if (!m_cache) {
            throw std::invalid_argument("CachedRepository: cache must not be null");
        }
    }

    /**
     * @brief 按 ID 查询(带缓存)
     *
     * 查询顺序:
     * 1. 缓存命中 → 直接返回
     * 2. 缓存未命中 → 查 delegate → 回填缓存
     * 3. delegate 返回 NotFound → 不回填,返回 Error
     */
    Result<T> findById(const Id& id) override {
        // 1. 查缓存
        auto cacheResult = m_cache->get(id);
        if (cacheResult.isOk()) {
            auto& opt = cacheResult.unwrap();
            if (opt.has_value()) {
                return Result<T>(*opt);
            }
        }
        // 缓存未命中或缓存故障:继续查 delegate

        // 2. 查 delegate
        auto delegateResult = m_delegate->findById(id);
        if (!delegateResult.isOk()) {
            // delegate 失败(包括 NotFound):不回填,直接返回错误
            return delegateResult.unwrapErr();
        }

        // 3. 回填缓存
        T& value = delegateResult.unwrap();
        (void)m_cache->put(id, value, m_ttl);

        return Result<T>(value);
    }

    /**
     * @brief 查询全部(不缓存,直接转发)
     */
    Result<std::vector<T>> findAll() override {
        return m_delegate->findAll();
    }

    /**
     * @brief 保存实体(转发 + 可选缓存更新)
     *
     * 若提供了 idExtractor,保存成功后会更新缓存;
     * 否则不触碰缓存(用户需手动失效)。
     */
    Result<T> save(const T& entity) override {
        auto result = m_delegate->save(entity);
        if (result.isOk() && m_idExtractor) {
            const T& saved = result.unwrap();
            Id id = m_idExtractor(saved);
            (void)m_cache->put(id, saved, m_ttl);
        }
        return result;
    }

    /**
     * @brief 按 ID 删除(转发 + 缓存失效)
     */
    Result<void> removeById(const Id& id) override {
        auto result = m_delegate->removeById(id);
        if (result.isOk()) {
            (void)m_cache->remove(id);
        }
        return result;
    }

    /**
     * @brief 批量保存(直接转发,不缓存)
     */
    Result<std::vector<T>> saveBatch(const std::vector<T>& entities) override {
        return m_delegate->saveBatch(entities);
    }

    /**
     * @brief 批量删除(转发 + 逐个失效缓存)
     */
    Result<void> removeBatch(const std::vector<Id>& ids) override {
        auto result = m_delegate->removeBatch(ids);
        if (result.isOk()) {
            for (const Id& id : ids) {
                (void)m_cache->remove(id);
            }
        }
        return result;
    }

    /**
     * @brief 计数(直接转发,不缓存)
     */
    Result<int> count() override {
        return m_delegate->count();
    }

    /**
     * @brief 清空整个缓存
     *
     * 不影响底层 delegate 数据。用于强制刷新场景。
     */
    Result<void> invalidateCache() {
        return m_cache->clear();
    }

    /**
     * @brief 获取缓存统计
     */
    [[nodiscard]] cache::CacheStats cacheStats() const {
        return m_cache->stats();
    }

private:
    std::shared_ptr<data::IRepository<T, Id>> m_delegate;
    std::shared_ptr<cache::ICache<Id, T>> m_cache;
    IdExtractor m_idExtractor;
    std::chrono::milliseconds m_ttl;
};

} // namespace orm
} // namespace sc

#endif // SOUL_ORM_CACHED_REPOSITORY_H
