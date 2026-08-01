#ifndef SOUL_ASYNC_AGGREGATE_H
#define SOUL_ASYNC_AGGREGATE_H

// soul_async.h — Async 模块聚合头文件
//
// 一行引入异步执行层:ThreadPool/TaskRunner/Future/Promise/Dispatcher/
// CancelableTask/AsyncRunner/Task。
//
// 用法:
//   #include "soul/soul_async.h"

#include "soul/async/thread_pool.h"
#include "soul/async/task.h"
#include "soul/async/task_runner.h"
#include "soul/async/future.h"
#include "soul/async/promise.h"
#include "soul/async/dispatcher.h"
#include "soul/async/cancelable_task.h"
#include "soul/async/async_runner.h"

#endif // SOUL_ASYNC_AGGREGATE_H
