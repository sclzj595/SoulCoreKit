#ifndef SOUL_CORE_AGGREGATE_H
#define SOUL_CORE_AGGREGATE_H

// soul_core.h — Core 模块聚合头文件
//
// 一行引入 Foundation Layer 全部能力:Result/Error/Interface/Singleton/
// Factory/Application/Platform/Time/Uuid/Version/Environment/Module/Scaffold。
//
// 用法:
//   #include "soul/soul_core.h"
//
// 设计参考: Qt 官方 <QtCore> 模块聚合头。

#include "soul/core/result.h"
#include "soul/core/error.h"
#include "soul/core/interface.h"
#include "soul/core/factory.h"
#include "soul/core/singleton.h"
#include "soul/core/version.h"
#include "soul/core/time.h"
#include "soul/core/uuid.h"
#include "soul/core/environment.h"
#include "soul/core/platform.h"
#include "soul/core/application.h"
#include "soul/core/module.h"
#include "soul/core/scaffold.h"

#endif // SOUL_CORE_AGGREGATE_H
