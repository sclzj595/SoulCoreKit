#ifndef SOUL_AGGREGATE_H
#define SOUL_AGGREGATE_H

// ============================================================================
// soul.h — SoulCoreKit 顶层聚合头文件
// ============================================================================
//
// 一行引入脚手架核心能力,对标 SpringBoot 的 @SpringBootApplication 一行接入。
//
// 设计原则(对标 SpringBoot starter 按需引入):
//   - soul.h 默认只聚合 Foundation Layer(Core/DI/Logging/Configuration)
//   - Infrastructure Layer(Async/Event/Network/Storage)按需引入:
//       #include "soul/soul_async.h"
//       #include "soul/soul_event.h"
//       #include "soul/soul_network.h"
//       #include "soul/soul_storage.h"
//   - Domain Layer(MQ/ORM/RPC/UI)按需引入:
//       #include "soul/soul_mq.h"
//       #include "soul/soul_orm.h"
//       #include "soul/soul_rpc.h"
//       #include "soul/ui/soul_ui.h"
//
// 用法:
//   #include "soul/soul.h"
//   int main(int argc, char* argv[]) {
//       sc::Scaffold scaffold(argc, argv);
//       return scaffold.run();
//   }
//
// 严格遵循"轻量骨架"定位:本头文件不引入任何 UI 依赖,
// 纯后端项目(CLI 工具/服务进程/Headless 服务)可安全使用。

#include "soul/soul_core.h"          // Foundation: Result/Error/Interface/Singleton/
                                     //   Factory/Application/Module/Scaffold/...
#include "soul/di/di_global.h"       // DI 容器全局宏
#include "soul/di/container.h"       // DI 容器
#include "soul/di/module.h"          // DI 模块

#include "soul/logging/log_macros.h" // SC_INFO/SC_ERROR/... 日志宏
#include "soul/logging/logger.h"     // Logger 单例

#include "soul/configuration/config.h"
#include "soul/configuration/iconfiguration.h"
#include "soul/configuration/json_configuration.h"
#include "soul/configuration/ini_configuration.h"

#include "soul/utils/string/string_utils.h"   // 通用字符串工具
#include "soul/utils/file/file_utils.h"       // 文件工具

#endif // SOUL_AGGREGATE_H
