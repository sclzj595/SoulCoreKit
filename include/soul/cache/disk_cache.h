#ifndef SOUL_CACHE_DISK_CACHE_H
#define SOUL_CACHE_DISK_CACHE_H

#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <QString>
#include "soul/cache/icache.h"

namespace sc {
namespace cache {

/**
 * @brief L2 磁盘缓存
 *
 * 将缓存条目持久化到磁盘文件,适合:
 * - 大容量数据(超出内存限制)
 * - 跨进程共享缓存
 * - 重启后仍可复用的数据
 *
 * @par 存储布局
 * 每个缓存键映射到两个文件:
 * - `<hash>.dat`: 值数据
 * - `<hash>.ttl`: 过期时间戳(仅当设置了 TTL 时存在)
 *
 * @par 键映射
 * 使用 SHA-256 哈希将任意键映射为固定长度文件名,避免非法字符问题。
 * 哈希前 2 字符作为子目录,分散文件避免单目录文件过多。
 *
 * @thread_safety Thread-Safe — 内部使用 std::mutex 同步所有访问
 */
class DiskCache : public ICache<std::string, std::string> {
public:
    /**
     * @brief 配置参数
     */
    struct Config {
        QString cacheDir;                                   ///< 缓存根目录
        std::size_t maxBytes = 1024 * 1024 * 1024;          ///< 最大字节数(默认 1GB)
        std::optional<std::chrono::milliseconds> defaultTtl;///< 默认 TTL
    };

    /**
     * @brief 构造函数
     * @param config 配置参数;cacheDir 必须非空,不存在时自动创建
     */
    explicit DiskCache(Config config);

    ~DiskCache() override = default;

    DiskCache(const DiskCache&) = delete;
    DiskCache& operator=(const DiskCache&) = delete;
    DiskCache(DiskCache&&) = delete;
    DiskCache& operator=(DiskCache&&) = delete;

    [[nodiscard]] Result<std::optional<std::string>> get(const std::string& key) override;
    Result<void> put(const std::string& key, const std::string& value,
                     std::optional<std::chrono::milliseconds> ttl = std::nullopt) override;
    Result<void> remove(const std::string& key) override;
    [[nodiscard]] Result<bool> contains(const std::string& key) override;
    Result<void> clear() override;
    [[nodiscard]] Result<std::unordered_map<std::string, std::string>>
        getMany(const std::vector<std::string>& keys) override;
    Result<void> putMany(const std::unordered_map<std::string, std::string>& entries,
                         std::optional<std::chrono::milliseconds> ttl = std::nullopt) override;

    [[nodiscard]] std::size_t size() const override;
    [[nodiscard]] CacheStats stats() const override;

private:
    /**
     * @brief 将键映射到数据文件绝对路径
     */
    QString keyToDataPath(const std::string& key) const;

    /**
     * @brief 将键映射到 TTL 文件绝对路径
     */
    QString keyToTtlPath(const std::string& key) const;

    /**
     * @brief 计算 SHA-256 哈希(十六进制)
     */
    QString computeHash(const std::string& key) const;

    /**
     * @brief 检查键是否已过期
     * @return true 表示过期或 TTL 文件损坏
     */
    bool isExpired(const std::string& key) const;

    /**
     * @brief 确保缓存目录存在
     */
    void ensureCacheDir() const;

    /**
     * @brief 启动时扫描磁盘恢复 totalBytes 和 LRU 索引
     * @details 遍历 cacheDir 下所有 .dat 文件,累加大小到 m_stats.totalBytes,
     *          并按文件 mtime 重建 m_lruIndex,使 maxBytes 限制在重启后仍生效
     */
    void recoverFromDiskUnlocked();

    /**
     * @brief 淘汰最久未访问的条目,直到能容纳 incomingSize
     * @param incomingSize 即将写入的字节数
     * @return 成功淘汰到足够空间返回 true;incomingSize 超 maxBytes 返回 false
     */
    bool evictToFitUnlocked(std::size_t incomingSize);

    /**
     * @brief 淘汰最久未访问的单个条目
     */
    void evictLruUnlocked();

    /**
     * @brief 更新键的最后访问时间(LRU 索引)
     */
    void touchUnlocked(const std::string& key);

    /**
     * @brief 原子写入文件(临时文件 + rename)
     * @param targetPath 目标文件路径
     * @param data 数据内容
     * @return 成功返回 true;失败时临时文件被清理,目标文件不受影响
     */
    bool writeAtomically(const QString& targetPath, const QByteArray& data);

    Config m_config;
    mutable std::mutex m_mutex;
    mutable CacheStats m_stats;

    /// LRU 索引:数据文件绝对路径 → 最后访问时间(基于 steady_clock,进程内单调)
    /// 用于 maxBytes 淘汰决策;重启后由 recoverFromDiskUnlocked 按文件 mtime 重建
    /// 使用文件路径而非原始 key 作为索引键,因为 recoverFromDiskUnlocked 无法从 hash
    /// 反推原始 key,但可以直接获取文件路径
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_lruIndex;
};

} // namespace cache
} // namespace sc

#endif // SOUL_CACHE_DISK_CACHE_H
