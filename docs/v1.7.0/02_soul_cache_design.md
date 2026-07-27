# 02 — SoulCache 模块设计

**优先级**: P1(推荐)
**模块名**: soul_cache
**命名空间**: `sc::cache`
**依赖**: soul_core, soul_storage, soul_logging

---

## 1. 设计目标

提供统一的缓存抽象,支持多级缓存(内存 → 磁盘 → 分布式),为 ORM 查询缓存与业务层热点数据提供一致的 API。

### 1.1 设计原则

1. **接口抽象**: 上层只依赖 `ICache<K,V>`,不感知具体实现
2. **多级透传**: L1(内存)未命中时自动查询 L2(磁盘),L2 未命中查询 L3(分布式)
3. **TTL + LRU 双策略**: 同时支持过期时间与容量淘汰
4. **线程安全**: 所有实现线程安全,符合 ADR-005 Level 2
5. **可观测**: 命中率/未命中率/延迟通过 SoulObservability 上报

---

## 2. 核心接口

### 2.1 ICache 抽象

```cpp
// include/soul/cache/icache.h
namespace sc::cache {

template<typename K, typename V>
class ICache {
public:
    virtual ~ICache() = default;

    // 同步 API
    virtual Result<std::optional<V>> get(const K& key) = 0;
    virtual Result<void> put(const K& key, const V& value,
                             std::optional<std::chrono::milliseconds> ttl = std::nullopt) = 0;
    virtual Result<void> remove(const K& key) = 0;
    virtual Result<bool> contains(const K& key) = 0;
    virtual Result<void> clear() = 0;

    // 批量 API
    virtual Result<std::unordered_map<K, V>> getMany(const std::vector<K>& keys) = 0;
    virtual Result<void> putMany(const std::unordered_map<K, V>& entries,
                                 std::optional<std::chrono::milliseconds> ttl = std::nullopt) = 0;

    // 元信息
    virtual size_t size() const = 0;
    virtual CacheStats stats() const = 0;
};

struct CacheStats {
    size_t hitCount = 0;
    size_t missCount = 0;
    size_t evictionCount = 0;
    size_t totalBytes = 0;
    double hitRate() const {
        size_t total = hitCount + missCount;
        return total == 0 ? 0.0 : static_cast<double>(hitCount) / total;
    }
};

} // namespace sc::cache
```

### 2.2 多级缓存

```cpp
// include/soul/cache/multi_level_cache.h
namespace sc::cache {

template<typename K, typename V>
class MultiLevelCache : public ICache<K, V> {
public:
    // 按顺序查询每一层,未命中则写入上层
    explicit MultiLevelCache(std::vector<std::shared_ptr<ICache<K, V>>> levels);

    Result<std::optional<V>> get(const K& key) override;
    Result<void> put(const K& key, const V& value,
                     std::optional<std::chrono::milliseconds> ttl = std::nullopt) override;
    // ... 其他方法
private:
    std::vector<std::shared_ptr<ICache<K, V>>> m_levels;
    mutable std::mutex m_mutex;
};

} // namespace sc::cache
```

### 2.3 L1 内存缓存

```cpp
// include/soul/cache/memory_cache.h
namespace sc::cache {

template<typename K, typename V>
class MemoryCache : public ICache<K, V> {
public:
    struct Config {
        size_t maxEntries = 10000;
        size_t maxBytes = 100 * 1024 * 1024;  // 100MB
        std::optional<std::chrono::milliseconds> defaultTtl;
    };

    explicit MemoryCache(const Config& config = {});
    ~MemoryCache() override;

    // ICache 实现...
private:
    struct Entry {
        V value;
        std::chrono::steady_clock::time_point expireTime;
        size_t size;
        size_t accessCount;
    };

    Config m_config;
    std::unordered_map<K, Entry> m_entries;
    mutable std::mutex m_mutex;
    mutable CacheStats m_stats;

    void evictIfNeeded();
    size_t estimateSize(const V& value) const;
};

} // namespace sc::cache
```

### 2.4 L2 磁盘缓存

```cpp
// include/soul/cache/disk_cache.h
namespace sc::cache {

class DiskCache : public ICache<std::string, std::string> {
public:
    struct Config {
        QString cacheDir;
        size_t maxBytes = 1024 * 1024 * 1024;  // 1GB
        std::optional<std::chrono::milliseconds> defaultTtl;
    };

    explicit DiskCache(const Config& config);
    ~DiskCache() override;

    // ICache 实现...
private:
    Config m_config;
    std::mutex m_mutex;
    CacheStats m_stats;

    QString keyToPath(const std::string& key) const;
    std::string computeHash(const std::string& key) const;
};

} // namespace sc::cache
```

---

## 3. 序列化策略

### 3.1 类型擦除的值存储

为支持任意类型,L2 磁盘缓存使用 `QVariant` 序列化:

```cpp
// 用户侧:自定义类型的磁盘缓存
class UserDiskCache : public DiskCache {
public:
    Result<std::optional<User>> getUser(int64_t id) {
        auto raw = get(std::to_string(id));
        if (!raw || !*raw) return std::nullopt;
        return User::deserialize(**raw);
    }
};
```

### 3.2 大小估算

`MemoryCache::estimateSize` 必须使用类型无关的方法(项目硬约束):
- 字符串: `value.size()`
- 容器: `sizeof(V) + sum(element sizes)`
- 复杂对象: 用户通过 `SizeEstimator<V>` 特化提供

```cpp
template<typename V>
struct SizeEstimator {
    static size_t estimate(const V& value) {
        return sizeof(V);  // 默认实现
    }
};

// 用户特化
template<>
struct SizeEstimator<std::vector<int>> {
    static size_t estimate(const std::vector<int>& v) {
        return sizeof(std::vector<int>) + v.size() * sizeof(int);
    }
};
```

---

## 4. 与 ORM 集成

### 4.1 查询缓存装饰器

```cpp
// include/soul/orm/cached_repository.h
namespace sc::orm {

template<typename T, typename KeyType = int64_t>
class CachedRepository : public IRepository<T, KeyType> {
public:
    CachedRepository(std::shared_ptr<IRepository<T, KeyType>> delegate,
                     std::shared_ptr<cache::ICache<KeyType, T>> cache,
                     std::chrono::milliseconds ttl = std::chrono::minutes(5));

    Result<std::optional<T>> findById(const KeyType& id) override {
        auto cached = m_cache->get(id);
        if (cached.isOk() && *cached) {
            return **cached;
        }
        auto result = m_delegate->findById(id);
        if (result.isOk() && *result) {
            m_cache->put(id, **result, m_ttl);
        }
        return result;
    }
    // ... 其他方法
private:
    std::shared_ptr<IRepository<T, KeyType>> m_delegate;
    std::shared_ptr<cache::ICache<KeyType, T>> m_cache;
    std::chrono::milliseconds m_ttl;
};

} // namespace sc::orm
```

---

## 5. CMake 集成

```cmake
# CMakeLists.txt 新增
add_library(soul_cache STATIC
    ${SOUL_CACHE_HEADERS}
    ${SOUL_CACHE_SOURCES}
)

target_include_directories(soul_cache PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_cache PUBLIC
    Qt6::Core
    soul_core
    soul_storage
    soul_logging
)
```

---

## 6. 测试策略

### 6.1 单元测试

| 测试类 | 覆盖点 |
|--------|--------|
| `TestMemoryCache` | get/put/remove/clear/TTL 过期/LRU 淘汰/容量限制 |
| `TestDiskCache` | 持久化/重启加载/磁盘满处理 |
| `TestMultiLevelCache` | L1 未命中查 L2/L2 命中回填 L1/全未命中 |
| `TestCachedRepository` | 装饰器模式/缓存失效/并发读写 |

### 6.2 性能基准

```cpp
// tests/bench_cache.cpp
void BenchmarkCache::testMemoryCacheThroughput() {
    MemoryCache<int, std::string> cache;
    QBENCHMARK {
        cache.put(42, "value");
        cache.get(42);
    }
}
```

目标: 内存缓存 get/put 单次操作 < 1μs

---

## 7. 范围决策

### 7.1 v1.7.0 范围(建议)

- ✅ `ICache` 抽象接口
- ✅ `MemoryCache`(L1)
- ✅ `DiskCache`(L2)
- ✅ `MultiLevelCache` 组合
- ✅ `CachedRepository` 装饰器
- ⏸️ 分布式缓存(L3)— 延后到 v1.8.0

### 7.2 待决策项

1. 是否在 v1.7.0 支持 Redis 作为 L3?
2. 序列化格式:QVariant vs Protocol Buffers vs FlatBuffers?
3. 是否支持缓存预热(warmup)?

---

## 8. 风险

| 风险 | 缓解 |
|------|------|
| 磁盘缓存在高并发下成为瓶颈 | 使用读写锁;考虑 mmap |
| 内存缓存大小估算不准导致 OOM | 默认保守估算;提供用户特化接口 |
| TTL 精度问题 | 使用 steady_clock;定时清理任务 |

---

**文档状态**: Draft
**最后更新**: 2026-07-26
