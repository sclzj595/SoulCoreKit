// ============================================================================
// test_v194_components.cpp — v1.9.4 组件综合测试
// ============================================================================
//
// 覆盖 v1.9.4 新增能力:
//   - 6 个 Actuator 端点 (Metrics/ThreadDump/Beans/Caches/ScheduledTasks/Shutdown)
//   - ThreadPool 细粒度优先级 (PriorityTask + std::priority_queue)
//   - ConnectionPool 动态扩缩容 (setDynamicResize)
//   - OtlpExporter OTLP JSON 序列化
//
// 设计原则: 每个测试用例自包含,不依赖全局可变状态(除单例 MetricsRegistry 外,
// 其指标名由测试唯一前缀 v194_test_ 隔离)。

#include <QTest>
#include <QCoreApplication>
#include <QThread>
#include <atomic>
#include <chrono>
#include <thread>

#include "soul/core/result.h"
#include "soul/server/metrics_endpoint.h"
#include "soul/server/threaddump_endpoint.h"
#include "soul/server/beans_endpoint.h"
#include "soul/server/caches_endpoint.h"
#include "soul/server/scheduledtasks_endpoint.h"
#include "soul/server/shutdown_endpoint.h"
#include "soul/async/thread_pool.h"
#include "soul/network/pool/connection_pool.h"
#include "soul/observability/tracing.h"
#include "soul/observability/metrics.h"
#include "soul/scheduler/scheduler.h"
#include "soul/scheduler/scheduled_task.h"
#include "soul/di/container.h"
#include "soul/utils/json/json_helper.h"

using namespace sc::server;
using namespace sc::observability;
using sc::ThreadPool;
using sc::Priority;
using sc::network::ConnectionPool;

// ============================================================================
// MetricsEndpoint 测试
// ============================================================================
class TestMetricsEndpoint : public QObject {
    Q_OBJECT
private slots:
    void testListMetricNames() {
        // 注册一个测试专用 Counter(唯一前缀避免与其他用例冲突)
        MetricsRegistry::instance().counter("v194_test_requests_total", "test counter");
        MetricsRegistry::instance().gauge("v194_test_cpu_usage", "test gauge");

        QByteArray json = MetricsEndpoint::listMetricNames();
        QVERIFY(!json.isEmpty());
        QVERIFY(json.contains("names"));
        QVERIFY(json.contains("v194_test_requests_total"));
        QVERIFY(json.contains("v194_test_cpu_usage"));
    }

    void testGetMetricByName() {
        MetricsRegistry::instance().counter("v194_test_counter_a", "test");
        MetricsRegistry::instance().counter("v194_test_counter_a", "test").increment(5);

        QByteArray json = MetricsEndpoint::getMetric("v194_test_counter_a");
        QVERIFY(!json.isEmpty());
        QVERIFY(json.contains("counter"));
        QVERIFY(json.contains("v194_test_counter_a"));
    }

    void testGetMetricNotFound() {
        QByteArray json = MetricsEndpoint::getMetric("v194_test_nonexistent_metric");
        QVERIFY(json.contains("metric not found"));
    }

    void testGetHistogramMetric() {
        HistogramBuckets buckets{{10, 100, 1000}};
        auto& hist = MetricsRegistry::instance().histogram(
            "v194_test_hist", "test histogram", buckets);
        hist.observe(50.0);
        hist.observe(500.0);

        QByteArray json = MetricsEndpoint::getMetric("v194_test_hist");
        QVERIFY(json.contains("histogram"));
        QVERIFY(json.contains("snapshot"));
    }
};

// ============================================================================
// ThreadDumpEndpoint 测试
// ============================================================================
class TestThreadDumpEndpoint : public QObject {
    Q_OBJECT
private slots:
    void testToJsonStructure() {
        QByteArray json = ThreadDumpEndpoint::toJson();
        QVERIFY(!json.isEmpty());
        QVERIFY(json.contains("threads"));
        QVERIFY(json.contains("hardwareConcurrency"));
        QVERIFY(json.contains("poolMaxThreadCount"));
    }

    void testCurrentThreadPresent() {
        QByteArray json = ThreadDumpEndpoint::toJson();
        // 当前线程必须出现在 threads 数组中
        QVERIFY(json.contains("isCurrent"));
        // 线程 id 字段存在
        QVERIFY(json.contains("id"));
    }
};

// ============================================================================
// BeansEndpoint 测试
// ============================================================================
class TestBeansEndpoint : public QObject {
    Q_OBJECT
private slots:
    void testToJsonStructure() {
        // 即便 DI 容器无注册,toJson 也应返回合法的 contexts 骨架
        QByteArray json = BeansEndpoint::toJson();
        QVERIFY(!json.isEmpty());
        QVERIFY(json.contains("contexts"));
        QVERIFY(json.contains("soulCoreKit"));
    }

    void testWithRegisteredBean() {
        // 注册一个测试 Bean,验证 beans 列表非空
        auto& container = sc::di::Container::instance();
        // bind<int> 已注册时会返回 AlreadyExists,忽略重复注册错误
        (void)container.bind<int>([]() -> int* { return new int(42); });

        QByteArray json = BeansEndpoint::toJson();
        // 至少应包含 type/scope/initialized 字段结构
        QVERIFY(json.contains("type"));
        QVERIFY(json.contains("scope"));
        QVERIFY(json.contains("initialized"));
    }
};

// ============================================================================
// CachesEndpoint 测试
// ============================================================================
class TestCachesEndpoint : public QObject {
    Q_OBJECT
private slots:
    void testRegisterAndQuery() {
        CachesEndpoint::clearRegistry();
        CachesEndpoint::registerCache("v194_test_userCache", 100, 80, 20, 0.8, 5);

        QByteArray json = CachesEndpoint::toJson();
        QVERIFY(json.contains("v194_test_userCache"));
        QVERIFY(json.contains("hitRate"));
        QVERIFY(json.contains("evictionCount"));
    }

    void testUnregisterCache() {
        CachesEndpoint::clearRegistry();
        CachesEndpoint::registerCache("v194_test_temp", 10, 5, 5, 0.5, 0);
        CachesEndpoint::unregisterCache("v194_test_temp");

        QByteArray json = CachesEndpoint::toJson();
        QVERIFY(!json.contains("v194_test_temp"));
    }

    void testClearRegistry() {
        CachesEndpoint::registerCache("v194_test_a", 1, 0, 0, 0.0, 0);
        CachesEndpoint::registerCache("v194_test_b", 2, 0, 0, 0.0, 0);
        CachesEndpoint::clearRegistry();

        QByteArray json = CachesEndpoint::toJson();
        QVERIFY(!json.contains("v194_test_a"));
        QVERIFY(!json.contains("v194_test_b"));
    }

    void testMultipleCaches() {
        CachesEndpoint::clearRegistry();
        CachesEndpoint::registerCache("v194_test_cache1", 10, 8, 2, 0.8, 1);
        CachesEndpoint::registerCache("v194_test_cache2", 20, 16, 4, 0.8, 2);

        QByteArray json = CachesEndpoint::toJson();
        QVERIFY(json.contains("v194_test_cache1"));
        QVERIFY(json.contains("v194_test_cache2"));
    }
};

// ============================================================================
// ScheduledTasksEndpoint 测试
// ============================================================================
class TestScheduledTasksEndpoint : public QObject {
    Q_OBJECT
private slots:
    void testEmptyScheduler() {
        sc::Scheduler scheduler;
        QByteArray json = ScheduledTasksEndpoint::toJson(scheduler);
        QVERIFY(!json.isEmpty());
        QVERIFY(json.contains("scheduledTasks"));
        // v3.0.0: serializePretty 使用 nlohmann dump(2), 冒号后带空格 (pretty 格式)
        QVERIFY(json.contains("\"total\": 0"));
    }

    void testWithCronTask() {
        sc::Scheduler scheduler;
        scheduler.addTask(sc::ScheduledTask::createCron(
            "0 */5 * * *", []() {}, "v194_test_cron_task"));

        QByteArray json = ScheduledTasksEndpoint::toJson(scheduler);
        QVERIFY(json.contains("v194_test_cron_task"));
        QVERIFY(json.contains("cron"));
        QVERIFY(json.contains("0 */5 * * *"));
    }

    void testWithFixedRateTask() {
        sc::Scheduler scheduler;
        scheduler.addTask(sc::ScheduledTask::createFixedRate(
            1000, []() {}, "v194_test_fixed_rate"));

        QByteArray json = ScheduledTasksEndpoint::toJson(scheduler);
        QVERIFY(json.contains("v194_test_fixed_rate"));
        QVERIFY(json.contains("fixedRate"));
        // v3.0.0: pretty 格式冒号后带空格
        QVERIFY(json.contains("\"intervalMs\": 1000"));
    }

    void testTaskCount() {
        sc::Scheduler scheduler;
        scheduler.addTask(sc::ScheduledTask::createFixedRate(
            500, []() {}, "v194_test_t1"));
        scheduler.addTask(sc::ScheduledTask::createFixedDelay(
            800, []() {}, "v194_test_t2"));

        QByteArray json = ScheduledTasksEndpoint::toJson(scheduler);
        // v3.0.0: pretty 格式冒号后带空格
        QVERIFY(json.contains("\"total\": 2"));
    }
};

// ============================================================================
// ShutdownEndpoint 测试
// ============================================================================
class TestShutdownEndpoint : public QObject {
    Q_OBJECT
private slots:
    void testToJsonContent() {
        QByteArray json = ShutdownEndpoint::toJson();
        QVERIFY(!json.isEmpty());
        QVERIFY(json.contains("Shutting down, bye..."));
        QVERIFY(json.contains("message"));
    }
};

// ============================================================================
// ThreadPool PriorityTask 测试 [v1.9.4]
// ============================================================================
class TestThreadPoolPriority : public QObject {
    Q_OBJECT
private slots:
    /// @brief [v1.9.4] 每个测试用例前重置 ThreadPool 单例,确保 worker 数可控
    void init() {
        ThreadPool::instance().shutdown();
    }

    /// @brief [v1.9.4] 测试结束后清理
    void cleanup() {
        ThreadPool::instance().shutdown();
    }

    void testSubmitPriorityExecutes() {
        ThreadPool& tp = ThreadPool::instance();
        tp.init(2);

        std::atomic<bool> executed{false};
        tp.submitPriority([&executed]() {
            executed.store(true);
        }, 10);

        // 等待任务执行完成(使用 waitForDone + 轮询)
        tp.waitForDone(3000);
        QVERIFY(executed.load());
    }

    void testPriorityQueueSizeAfterCompletion() {
        ThreadPool& tp = ThreadPool::instance();
        tp.init(2);

        // 提交若干任务并等待全部完成
        std::atomic<int> count{0};
        for (int i = 0; i < 5; ++i) {
            tp.submitPriority([&count]() {
                count.fetch_add(1);
            }, i);
        }

        tp.waitForDone(3000);
        QCOMPARE(count.load(), 5);
        // 全部完成后 priority_queue 应为空
        QCOMPARE(tp.priorityQueueSize(), 0);
    }

    void testQueueSizeIncludesPriorityQueue() {
        ThreadPool& tp = ThreadPool::instance();
        tp.init(2);

        // 用 barrier 占住所有工作线程(init 是幂等的,worker 数 = maxThreadCount)
        std::atomic<bool> release{false};
        std::atomic<int> startedCount{0};
        int workerCount = tp.maxThreadCount();
        auto barrier = [&release, &startedCount]() {
            startedCount.fetch_add(1);
            while (!release.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        };
        for (int i = 0; i < workerCount; ++i) {
            tp.start(barrier);
        }

        // 等待所有工作线程进入 barrier
        while (startedCount.load() < workerCount) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        // 工作线程被阻塞,提交的优先级任务应留在队列中
        tp.submitPriority([]() {}, 1);
        tp.submitPriority([]() {}, 2);
        QVERIFY(tp.priorityQueueSize() >= 1);
        QVERIFY(tp.queueSize() >= 1);

        release.store(true);
        tp.waitForDone(3000);
        QCOMPARE(tp.priorityQueueSize(), 0);
    }

    void testHighPriorityPreemptsHighQueue() {
        // 验证 priority >= High(3) 的任务优先于 High 队列执行
        ThreadPool& tp = ThreadPool::instance();
        tp.init(2);

        std::atomic<int> order{0};
        int highOrder = -1;
        int priorityOrder = -1;

        // 用 barrier 占住所有工作线程,确保 High 与 priority 任务同时入队
        std::atomic<bool> release{false};
        std::atomic<int> startedCount{0};
        int workerCount = tp.maxThreadCount();
        auto barrier = [&release, &startedCount]() {
            startedCount.fetch_add(1);
            while (!release.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        };
        for (int i = 0; i < workerCount; ++i) {
            tp.start(barrier);
        }
        while (startedCount.load() < workerCount) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        // 入队顺序: 先 High 队列,后 priority_queue(优先级 100,>= High)
        tp.start([&order, &highOrder]() {
            highOrder = order.fetch_add(1);
        }, Priority::High);

        tp.submitPriority([&order, &priorityOrder]() {
            priorityOrder = order.fetch_add(1);
        }, 100);

        // 释放 barrier,工作线程消费:priority=100 先于 High(dequeueTask 优先级规则)
        release.store(true);
        tp.waitForDone(3000);

        QVERIFY(priorityOrder >= 0);
        QVERIFY(highOrder >= 0);
        QVERIFY(priorityOrder < highOrder);
    }
};

// ============================================================================
// ConnectionPool 动态扩缩容 测试 [v1.9.4]
// ============================================================================
class TestConnectionPoolDynamicResize : public QObject {
    Q_OBJECT
private slots:
    void testSetDynamicResize() {
        ConnectionPool::Config cfg;
        cfg.minConnections = 1;
        cfg.maxConnections = 5;
        ConnectionPool pool(cfg);

        pool.setDynamicResize(3, 10);

        // 验证配置已生效(通过 isDynamicResizeEnabled 间接验证)
        QVERIFY(pool.isDynamicResizeEnabled());
    }

    void testIsDynamicResizeEnabled() {
        ConnectionPool::Config cfg;
        cfg.minConnections = 0;
        cfg.maxConnections = 5;
        ConnectionPool pool(cfg);

        // min=0 时未启用动态扩缩容
        QVERIFY(!pool.isDynamicResizeEnabled());

        pool.setDynamicResize(2, 8);
        QVERIFY(pool.isDynamicResizeEnabled());
    }

    void testInvalidDynamicResizeConfig() {
        ConnectionPool::Config cfg;
        ConnectionPool pool(cfg);

        // min >= max 时不应视为启用
        pool.setDynamicResize(5, 5);
        QVERIFY(!pool.isDynamicResizeEnabled());

        pool.setDynamicResize(8, 5);
        QVERIFY(!pool.isDynamicResizeEnabled());
    }
};

// ============================================================================
// OtlpExporter 测试 [v1.9.4]
// ============================================================================
class TestOtlpExporter : public QObject {
    Q_OBJECT
private slots:
    void testSerializeEmptySpans() {
        std::vector<std::shared_ptr<Span>> empty;
        QByteArray json = OtlpExporter::serializeSpans(empty);
        QVERIFY(!json.isEmpty());
        QVERIFY(json.contains("resourceSpans"));
    }

    void testSerializeEndedSpan() {
        Tracer::instance().setEnabled(true);
        auto span = Tracer::instance().startSpan("v194_test_otlp_span");
        QVERIFY(span != nullptr);
        span->setTag("component", "test");
        span->addEvent("test_event", {{"key", "value"}});
        span->setStatus(true, "ok");
        span->end();

        std::vector<std::shared_ptr<Span>> spans = {span};
        QByteArray json = OtlpExporter::serializeSpans(spans);

        QVERIFY(json.contains("resourceSpans"));
        QVERIFY(json.contains("v194_test_otlp_span"));
        QVERIFY(json.contains("traceId"));
        QVERIFY(json.contains("spanId"));
        QVERIFY(json.contains("startTimeUnixNano"));
        QVERIFY(json.contains("endTimeUnixNano"));
        QVERIFY(json.contains("attributes"));
    }

    void testSerializeSkipsUnendedSpan() {
        Tracer::instance().setEnabled(true);
        auto unended = Tracer::instance().startSpan("v194_test_unended");
        auto ended = Tracer::instance().startSpan("v194_test_ended");
        ended->end();

        std::vector<std::shared_ptr<Span>> spans = {unended, ended};
        QByteArray json = OtlpExporter::serializeSpans(spans);

        // 未结束的 span 应被跳过
        QVERIFY(json.contains("v194_test_ended"));
        QVERIFY(!json.contains("v194_test_unended"));

        if (unended) unended->end();
    }

    void testExporterEndpointAccessors() {
        OtlpExporter exporter("http://localhost:4318/v1/traces");
        QCOMPARE(exporter.endpoint(), std::string("http://localhost:4318/v1/traces"));

        exporter.setEndpoint("http://collector:4318/v1/traces");
        QCOMPARE(exporter.endpoint(), std::string("http://collector:4318/v1/traces"));
    }

    void testExportSpansReturnsJson() {
        Tracer::instance().setEnabled(true);
        auto span = Tracer::instance().startSpan("v194_test_export");
        span->end();

        OtlpExporter exporter;
        std::vector<std::shared_ptr<Span>> spans = {span};
        QByteArray json = exporter.serialize(spans);

        QVERIFY(!json.isEmpty());
        QVERIFY(json.contains("resourceSpans"));
    }
};

// ============================================================================
// main
// ============================================================================
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);
    int status = 0;
    {
        TestMetricsEndpoint tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestThreadDumpEndpoint tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestBeansEndpoint tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestCachesEndpoint tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestScheduledTasksEndpoint tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestShutdownEndpoint tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestThreadPoolPriority tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestConnectionPoolDynamicResize tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestOtlpExporter tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
#include "test_v194_components.moc"
