#ifndef SOUL_SERVER_CACHES_ENDPOINT_H
#define SOUL_SERVER_CACHES_ENDPOINT_H

// ============================================================================
// caches_endpoint.h — 缓存内省端点 [v1.9.4]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/caches,暴露应用中所有缓存的状态信息
// (大小/命中率/淘汰次数等)。
//
// 由于 ICache 是模板类,缺乏统一的注册表,本端点维护一个独立的缓存注册表,
// 供应用启动时通过 registerCache() 注册各缓存实例的统计快照。
//
// 用法:
//   // 应用启动时注册缓存
//   CachesEndpoint::registerCache("userCache", cache.size(), cache.hits(),
//                                 cache.misses(), cache.hitRate(), cache.evictions());
//
//   // 暴露端点
//   server.get("/actuator/caches", [](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(CachesEndpoint::toJson());
//   });

#include "soul/utils/json/json_helper.h"

#include <QByteArray>
#include <cstddef>
#include <map>
#include <mutex>
#include <string>

namespace sc {
namespace server {

// ============================================================================
// CachesEndpoint — 缓存内省端点
// ============================================================================
class CachesEndpoint {
public:
    // 单个缓存的统计信息条目
    struct CacheEntry {
        std::string name;
        std::size_t size = 0;
        std::size_t hitCount = 0;
        std::size_t missCount = 0;
        double hitRate = 0.0;
        std::size_t evictionCount = 0;
    };

    /// @brief 注册缓存(供应用启动时调用)
    /// @thread_safety 线程安全
    static void registerCache(const std::string& name, std::size_t size,
                              std::size_t hitCount, std::size_t missCount,
                              double hitRate, std::size_t evictionCount) {
        std::lock_guard<std::mutex> lock(s_mutex);
        CacheEntry entry{name, size, hitCount, missCount, hitRate, evictionCount};
        s_caches[name] = entry;
    }

    /// @brief 注销缓存
    /// @thread_safety 线程安全
    static void unregisterCache(const std::string& name) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_caches.erase(name);
    }

    /// @brief 清空注册表(用于测试)
    /// @thread_safety 线程安全
    static void clearRegistry() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_caches.clear();
    }

    /// @brief 列出所有缓存并序列化为 JSON
    /// @thread_safety 线程安全
    static QByteArray toJson() {
        std::lock_guard<std::mutex> lock(s_mutex);
        sc::json::Json root = sc::json::Json::object();
        sc::json::Json cacheGroups = sc::json::Json::object();
        sc::json::Json cacheList = sc::json::Json::array();

        // 遍历注册表,构建每个缓存条目
        for (const auto& [name, entry] : s_caches) {
            sc::json::Json cache = sc::json::Json::object();
            cache["name"] = name;
            cache["size"] = entry.size;
            cache["hitCount"] = entry.hitCount;
            cache["missCount"] = entry.missCount;
            cache["hitRate"] = entry.hitRate;
            cache["evictionCount"] = entry.evictionCount;
            cacheList.push_back(cache);
        }

        // 按 SpringBoot 风格组织: contexts.<contextName>.[]
        cacheGroups["soulCoreKit"] = cacheList;
        root["contexts"] = cacheGroups;
        root["caches"] = cacheList;  // 兼容格式
        return sc::json::serializePretty(root);
    }

private:
    inline static std::mutex s_mutex;
    inline static std::map<std::string, CacheEntry> s_caches;
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_CACHES_ENDPOINT_H
