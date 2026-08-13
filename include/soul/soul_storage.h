#ifndef SOUL_STORAGE_AGGREGATE_H
#define SOUL_STORAGE_AGGREGATE_H

// soul_storage.h — Storage 模块聚合头文件 [v2.9.4]
//
// 一行引入存储层:IStorage/MemoryStorage/FileStorage/SqliteDatabase/Cache/
// ISerializer/JsonSerializer/Settings。
//
// v2.9.4: 新增 soul/cache/ 聚合 (Track B)，旧 storage/cache.h 标记 deprecated。
//
// 用法:
//   #include "soul/soul_storage.h"

// --- 存储核心 ---
#include "soul/storage/istorage.h"
#include "soul/storage/memory_storage.h"
#include "soul/storage/file_storage.h"
#include "soul/storage/sqlite_database.h"
#include "soul/storage/i_serializer.h"
#include "soul/storage/json_serializer.h"
#include "soul/storage/settings.h"

// --- 缓存 (canonical) ---
#include "soul/cache/memory_cache.h"
#include "soul/cache/disk_cache.h"
#include "soul/cache/multi_level_cache.h"

#endif // SOUL_STORAGE_AGGREGATE_H
