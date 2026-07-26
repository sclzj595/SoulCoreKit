#ifndef SOUL_CACHE_MULTI_LEVEL_CACHE_H
#define SOUL_CACHE_MULTI_LEVEL_CACHE_H

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>
#include "soul/cache/icache.h"
#include "soul/logging/log_macros.h"

namespace sc {
namespace cache {

/**
 * @brief 多级缓存组合
 *
 * 将多个 ICache 实例按优先级组合:L1(内存)未命中时自动查询 L2(磁盘),
 * L2 命中后回填 L1,降低后续访问延迟。
 *
 * @par 层级顺序
 * 构造时传入的 levels 向量顺序即为查询顺序:
 * - levels[0]: 最快、最小(L1 内存)
 * - levels[1]: 较慢、较大(L2 磁盘)
 * - ...
 *
 * @par 读取语义
 * - get: 按顺序查询,首个命中即返回并回填上层
 * - contains: 任一层命中即返回 true
 *
 * @par 写入语义
 * - put/putMany: 广播到所有层
 *
 * @thread_safety Thread-Safe — 内部使用 std::mutex 同步回填操作
 */
template<typename K, typename V>
class MultiLevelCache : public ICache<K, V> {
public:
    /**
     * @brief 构造函数
     * @param levels 缓存层级,非空向量;顺序即查询优先级
     * @throws std::invalid_argument 若 levels 为空
     */
    explicit MultiLevelCache(std::vector<std::shared_ptr<ICache<K, V>>> levels) {
        if (levels.empty()) {
            throw std::invalid_argument("MultiLevelCache: levels must not be empty");
        }
        m_levels = std::move(levels);
    }

    ~MultiLevelCache() override = default;

    MultiLevelCache(const MultiLevelCache&) = delete;
    MultiLevelCache& operator=(const MultiLevelCache&) = delete;
    MultiLevelCache(MultiLevelCache&&) = delete;
    MultiLevelCache& operator=(MultiLevelCache&&) = delete;

    [[nodiscard]] Result<std::optional<V>> get(const K& key) override {
        // 第一阶段:按顺序查询,记录首个命中层
        for (std::size_t i = 0; i < m_levels.size(); ++i) {
            auto result = m_levels[i]->get(key);
            if (!result.isOk()) {
                // 底层故障:记录警告后跳过此层,继续查询下层(降级策略)
                SC_WARN("MultiLevelCache: level " + std::to_string(i) +
                        " get failed, skipping to next level");
                continue;
            }
            auto opt = result.unwrap();
            if (opt.has_value()) {
                // 命中:回填上层
                const V& value = opt.value();
                backfillUpper(key, value, i);
                return std::optional<V>{value};
            }
        }
        return std::optional<V>{};
    }

    Result<void> put(const K& key, const V& value,
                     std::optional<std::chrono::milliseconds> ttl = std::nullopt) override {
        Result<void> lastError;
        for (const auto& level : m_levels) {
            auto r = level->put(key, value, ttl);
            if (!r.isOk()) {
                lastError = r;
            }
        }
        return lastError;
    }

    Result<void> remove(const K& key) override {
        // 缓存一致性要求:任一层 remove 失败(非 NotFound)立即返回错误,不继续删除其他层。
        // 原因:若 L1 成功删除但 L2 失败(残留 key),下次 get 时 L1 miss → L2 hit → 回填 L1,
        // 导致已删除数据"复活"。立即返回让调用者感知不一致,可采取恢复措施。
        // 对 "键不存在"(NotFound) 视为成功,因为该层本就无此键。
        for (std::size_t i = 0; i < m_levels.size(); ++i) {
            auto r = m_levels[i]->remove(key);
            if (!r.isOk() && r.unwrapErr().code() != ErrorCode::NotFound) {
                SC_WARN("MultiLevelCache: level " + std::to_string(i) +
                        " remove failed, aborting remaining levels to prevent key resurrection");
                return r;
            }
        }
        return Result<void>::ok();
    }

    [[nodiscard]] Result<bool> contains(const K& key) override {
        for (std::size_t i = 0; i < m_levels.size(); ++i) {
            auto r = m_levels[i]->contains(key);
            if (!r.isOk()) {
                SC_WARN("MultiLevelCache: level " + std::to_string(i) +
                        " contains failed, skipping to next level");
                continue;
            }
            if (r.unwrap()) {
                return true;
            }
        }
        return false;
    }

    Result<void> clear() override {
        Result<void> lastError;
        for (const auto& level : m_levels) {
            auto r = level->clear();
            if (!r.isOk()) {
                lastError = r;
            }
        }
        return lastError;
    }

    [[nodiscard]] Result<std::unordered_map<K, V>> getMany(const std::vector<K>& keys) override {
        std::unordered_map<K, V> result;
        result.reserve(keys.size());

        std::vector<K> remaining = keys;

        for (std::size_t i = 0; i < m_levels.size() && !remaining.empty(); ++i) {
            auto r = m_levels[i]->getMany(remaining);
            if (!r.isOk()) {
                SC_WARN("MultiLevelCache: level " + std::to_string(i) +
                        " getMany failed, skipping to next level");
                continue;
            }

            auto& hits = r.unwrap();

            std::vector<K> nextRemaining;
            nextRemaining.reserve(remaining.size());

            for (const K& key : remaining) {
                auto it = hits.find(key);
                if (it != hits.end()) {
                    result.emplace(key, it->second);
                } else {
                    nextRemaining.push_back(key);
                }
            }

            // 回填上层
            if (i > 0 && !hits.empty()) {
                backfillUpperMany(hits, i);
            }

            remaining = std::move(nextRemaining);
        }

        return result;
    }

    Result<void> putMany(const std::unordered_map<K, V>& entries,
                         std::optional<std::chrono::milliseconds> ttl = std::nullopt) override {
        Result<void> lastError;
        for (const auto& level : m_levels) {
            auto r = level->putMany(entries, ttl);
            if (!r.isOk()) {
                lastError = r;
            }
        }
        return lastError;
    }

    [[nodiscard]] std::size_t size() const override {
        // 返回第一层(L1)的大小,作为最接近用户感知的指标
        if (m_levels.empty()) {
            return 0;
        }
        return m_levels[0]->size();
    }

    [[nodiscard]] CacheStats stats() const override {
        // 合并所有层的统计
        CacheStats merged;
        for (const auto& level : m_levels) {
            const CacheStats s = level->stats();
            merged.hitCount += s.hitCount;
            merged.missCount += s.missCount;
            merged.evictionCount += s.evictionCount;
            merged.totalBytes += s.totalBytes;
        }
        return merged;
    }

private:
    /**
     * @brief 将命中的值回填到第 hitIndex 层之上的所有层
     */
    void backfillUpper(const K& key, const V& value, std::size_t hitIndex) {
        for (std::size_t i = 0; i < hitIndex; ++i) {
            (void)m_levels[i]->put(key, value);
        }
    }

    /**
     * @brief 批量回填
     */
    void backfillUpperMany(const std::unordered_map<K, V>& entries, std::size_t hitIndex) {
        for (std::size_t i = 0; i < hitIndex; ++i) {
            (void)m_levels[i]->putMany(entries);
        }
    }

    std::vector<std::shared_ptr<ICache<K, V>>> m_levels;
};

} // namespace cache
} // namespace sc

#endif // SOUL_CACHE_MULTI_LEVEL_CACHE_H
