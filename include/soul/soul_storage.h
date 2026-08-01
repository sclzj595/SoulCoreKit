#ifndef SOUL_STORAGE_AGGREGATE_H
#define SOUL_STORAGE_AGGREGATE_H

// soul_storage.h — Storage 模块聚合头文件
//
// 一行引入存储层:IStorage/MemoryStorage/FileStorage/SqliteDatabase/Cache/
// ISerializer/JsonSerializer/Settings。
//
// 用法:
//   #include "soul/soul_storage.h"

#include "soul/storage/istorage.h"
#include "soul/storage/memory_storage.h"
#include "soul/storage/file_storage.h"
#include "soul/storage/sqlite_database.h"
#include "soul/storage/cache.h"
#include "soul/storage/i_serializer.h"
#include "soul/storage/json_serializer.h"
#include "soul/storage/settings.h"

#endif // SOUL_STORAGE_AGGREGATE_H
