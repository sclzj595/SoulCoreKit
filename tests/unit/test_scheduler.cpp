#include <QTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimer>

#include "soul/scheduler/scheduled_task.h"
#include "soul/scheduler/scheduler.h"

using namespace sc;

// ============================================================================
// ScheduledTask — FixedRate
// ============================================================================
class TestScheduledTaskFixedRate : public QObject {
    Q_OBJECT
private slots:
    void testCreateFixedRate() {
        auto task = ScheduledTask::createFixedRate(1000, []() {});
        QVERIFY(task != nullptr);
        QCOMPARE(task->mode(), ScheduledTask::Mode::FixedRate);
        QCOMPARE(task->intervalMs(), 1000);
        QVERIFY(!task->isRunning());
        QCOMPARE(task->executionCount(), 0);
    }

    void testCreateFixedDelay() {
        auto task = ScheduledTask::createFixedDelay(500, []() {});
        QVERIFY(task != nullptr);
        QCOMPARE(task->mode(), ScheduledTask::Mode::FixedDelay);
        QCOMPARE(task->intervalMs(), 500);
    }

    void testCreateCron() {
        auto task = ScheduledTask::createCron("0 */5 * * *", []() {});
        QVERIFY(task != nullptr);
        QCOMPARE(task->mode(), ScheduledTask::Mode::Cron);
        QCOMPARE(task->cronExpression(), std::string("0 */5 * * *"));
    }

    void testName() {
        auto task = ScheduledTask::createFixedRate(1000, []() {}, "myTask");
        QCOMPARE(task->name(), std::string("myTask"));
    }

    void testEmptyName() {
        auto task = ScheduledTask::createFixedRate(1000, []() {});
        QVERIFY(task->name().empty());
    }

    void testStartStop() {
        auto task = ScheduledTask::createFixedRate(1000, []() {});
        QVERIFY(!task->isRunning());
        task->start();
        QVERIFY(task->isRunning());
        task->stop();
        QVERIFY(!task->isRunning());
    }

    void testDoubleStart() {
        auto task = ScheduledTask::createFixedRate(1000, []() {});
        task->start();
        task->start(); // 不应有副作用
        QVERIFY(task->isRunning());
        task->stop();
    }

    void testTriggerNow() {
        int count = 0;
        auto task = ScheduledTask::createFixedRate(10000, [&count]() { count++; });
        QCOMPARE(task->executionCount(), 0);
        task->triggerNow();
        QCOMPARE(count, 1);
        QCOMPARE(task->executionCount(), 1);
    }

    void testStartedSignal() {
        auto task = ScheduledTask::createFixedRate(1000, []() {});
        QSignalSpy spy(task.get(), &ScheduledTask::started);
        task->start();
        QCOMPARE(spy.count(), 1);
        task->stop();
    }

    void testStoppedSignal() {
        auto task = ScheduledTask::createFixedRate(1000, []() {});
        task->start();
        QSignalSpy spy(task.get(), &ScheduledTask::stopped);
        task->stop();
        QCOMPARE(spy.count(), 1);
    }

    void testExecutedSignal() {
        auto task = ScheduledTask::createFixedRate(10000, []() {});
        QSignalSpy spy(task.get(), &ScheduledTask::executed);
        task->triggerNow();
        QCOMPARE(spy.count(), 1);
    }

    void testLastExecutionTime() {
        auto task = ScheduledTask::createFixedRate(10000, []() {});
        QVERIFY(!task->lastExecutionTime().isValid());
        task->triggerNow();
        QVERIFY(task->lastExecutionTime().isValid());
    }
};

// ============================================================================
// ScheduledTask — Cron
// ============================================================================
class TestScheduledTaskCron : public QObject {
    Q_OBJECT
private slots:
    void testCronEveryMinute() {
        auto task = ScheduledTask::createCron("* * * * *", []() {});
        QCOMPARE(task->mode(), ScheduledTask::Mode::Cron);
    }

    void testCronDaily() {
        auto task = ScheduledTask::createCron("0 0 * * *", []() {});
        QCOMPARE(task->mode(), ScheduledTask::Mode::Cron);
    }

    void testCronWeekday() {
        auto task = ScheduledTask::createCron("0 8 * * 1-5", []() {});
        QCOMPARE(task->mode(), ScheduledTask::Mode::Cron);
    }

    void testCronInvalidExpression() {
        // 无效表达式不应崩溃
        auto task = ScheduledTask::createCron("invalid", []() {});
        QVERIFY(task != nullptr);
    }
};

// ============================================================================
// Scheduler
// ============================================================================
class TestScheduler : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Scheduler s;
        QCOMPARE(s.taskCount(), 0);
        QVERIFY(s.isAllStopped());
    }

    void testAddTask() {
        Scheduler s;
        auto task = ScheduledTask::createFixedRate(1000, []() {}, "t1");
        s.addTask(task);
        QCOMPARE(s.taskCount(), 1);
        QCOMPARE(s.findTask("t1")->name(), std::string("t1"));
    }

    void testAddNullTask() {
        Scheduler s;
        s.addTask(nullptr);
        QCOMPARE(s.taskCount(), 0);
    }

    void testAddDuplicateName() {
        Scheduler s;
        auto t1 = ScheduledTask::createFixedRate(1000, []() {}, "dup");
        auto t2 = ScheduledTask::createFixedRate(2000, []() {}, "dup");
        s.addTask(t1);
        s.addTask(t2);
        QCOMPARE(s.taskCount(), 1);
        QCOMPARE(s.findTask("dup")->intervalMs(), 2000);
    }

    void testRemoveTask() {
        Scheduler s;
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "t1"));
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "t2"));
        QCOMPARE(s.taskCount(), 2);
        s.removeTask("t1");
        QCOMPARE(s.taskCount(), 1);
        QVERIFY(s.findTask("t1") == nullptr);
        QVERIFY(s.findTask("t2") != nullptr);
    }

    void testRemoveNonexistent() {
        Scheduler s;
        s.removeTask("nonexistent");
        QCOMPARE(s.taskCount(), 0);
    }

    void testAllTasks() {
        Scheduler s;
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "a"));
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "b"));
        auto tasks = s.allTasks();
        QCOMPARE(tasks.size(), 2);
    }

    void testStartAllStopAll() {
        Scheduler s;
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "a"));
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "b"));
        s.startAll();
        QVERIFY(!s.isAllStopped());
        s.stopAll();
        QVERIFY(s.isAllStopped());
    }

    void testStartStopTask() {
        Scheduler s;
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "a"));
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "b"));
        s.startTask("a");
        QVERIFY(s.findTask("a")->isRunning());
        QVERIFY(!s.findTask("b")->isRunning());
        s.stopTask("a");
        QVERIFY(!s.findTask("a")->isRunning());
    }

    void testTotalExecutionCount() {
        Scheduler s;
        auto t1 = ScheduledTask::createFixedRate(10000, []() {}, "a");
        auto t2 = ScheduledTask::createFixedRate(10000, []() {}, "b");
        s.addTask(t1);
        s.addTask(t2);
        t1->triggerNow();
        t2->triggerNow();
        t2->triggerNow();
        QCOMPARE(s.totalExecutionCount(), 3);
    }

    void testTaskAddedSignal() {
        Scheduler s;
        QSignalSpy spy(&s, &Scheduler::taskAdded);
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "a"));
        QCOMPARE(spy.count(), 1);
    }

    void testTaskRemovedSignal() {
        Scheduler s;
        s.addTask(ScheduledTask::createFixedRate(1000, []() {}, "a"));
        QSignalSpy spy(&s, &Scheduler::taskRemoved);
        s.removeTask("a");
        QCOMPARE(spy.count(), 1);
    }

    void testAllStartedSignal() {
        Scheduler s;
        QSignalSpy spy(&s, &Scheduler::allStarted);
        s.startAll();
        QCOMPARE(spy.count(), 1);
    }

    void testAllStoppedSignal() {
        Scheduler s;
        s.startAll();
        QSignalSpy spy(&s, &Scheduler::allStopped);
        s.stopAll();
        QCOMPARE(spy.count(), 1);
    }
};

// ============================================================================
// FixedDelay 行为验证
// ============================================================================
class TestScheduledTaskBehavior : public QObject {
    Q_OBJECT
private slots:
    void testFixedRateExecution() {
        int count = 0;
        auto task = ScheduledTask::createFixedRate(100, [&count]() { count++; });
        task->start();
        QTest::qWait(350);
        task->stop();
        QVERIFY(count >= 2);
    }

    void testFixedDelayExecution() {
        int count = 0;
        auto task = ScheduledTask::createFixedDelay(100, [&count]() { count++; });
        task->start();
        QTest::qWait(350);
        task->stop();
        QVERIFY(count >= 2);
    }

    void testStopPreventsExecution() {
        int count = 0;
        auto task = ScheduledTask::createFixedRate(500, [&count]() { count++; });
        task->start();
        QTest::qWait(100);
        task->stop();
        int countAfterStop = count;
        QTest::qWait(600);
        QCOMPARE(count, countAfterStop);
    }
};

// ============================================================================
// main
// ============================================================================
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    int result = 0;

    { TestScheduledTaskFixedRate t; result |= QTest::qExec(&t, argc, argv); }
    { TestScheduledTaskCron t; result |= QTest::qExec(&t, argc, argv); }
    { TestScheduler t; result |= QTest::qExec(&t, argc, argv); }
    { TestScheduledTaskBehavior t; result |= QTest::qExec(&t, argc, argv); }

    return result;
}

#include "test_scheduler.moc"