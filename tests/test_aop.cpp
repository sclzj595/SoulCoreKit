// ============================================================================
// test_aop.cpp — AOP 切面编程单元测试
// ============================================================================
//
// 覆盖范围:
//   - Pointcut 匹配(前缀/后缀/包含/精确)
//   - Before/After/AfterReturning/AfterThrowing/Around advice
//   - AspectWeaver 注册/注销/织入
//   - 多切面组合织入顺序
//   - Around advice 控制 proceed(可跳过目标方法)

#include <QTest>
#include <QString>
#include <stdexcept>
#include <string>
#include <vector>

#include "soul/aop/aop.h"

using namespace sc;
using namespace sc::aop;

// ============================================================================
// TestAop — AOP 测试
// ============================================================================
class TestAop : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();

    void testPointcutPrefix();
    void testPointcutSuffix();
    void testPointcutContains();
    void testPointcutExact();
    void testBeforeAdvice();
    void testAfterAdvice();
    void testAfterReturningAdvice();
    void testAfterThrowingAdvice();
    void testExceptionTypePreservation();
    void testAroundAdvice();
    void testAroundAdviceCanSkipTarget();
    void testMultipleAspectsOrder();
    void testRegisterUnregister();
};

void TestAop::initTestCase() {
    AspectWeaver::instance().clear();
}

void TestAop::cleanup() {
    AspectWeaver::instance().clear();
}

// ============================================================================
// Pointcut 匹配测试
// ============================================================================
void TestAop::testPointcutPrefix() {
    auto pc = Pointcut::byPrefix("UserService::");
    QVERIFY(pc.matches("UserService::login"));
    QVERIFY(pc.matches("UserService::logout"));
    QVERIFY(!pc.matches("OrderService::create"));
}

void TestAop::testPointcutSuffix() {
    auto pc = Pointcut::bySuffix("::login");
    QVERIFY(pc.matches("UserService::login"));
    QVERIFY(pc.matches("AuthService::login"));
    QVERIFY(!pc.matches("UserService::logout"));
}

void TestAop::testPointcutContains() {
    auto pc = Pointcut::byContains("Service");
    QVERIFY(pc.matches("UserService::login"));
    QVERIFY(pc.matches("OrderService::create"));
    QVERIFY(!pc.matches("Repository::find"));
}

void TestAop::testPointcutExact() {
    auto pc = Pointcut::byExact("UserService::login");
    QVERIFY(pc.matches("UserService::login"));
    QVERIFY(!pc.matches("UserService::logout"));
}

// ============================================================================
// Before/After advice 测试
// ============================================================================
void TestAop::testBeforeAdvice() {
    Aspect aspect("before_test");
    aspect.setPointcut(Pointcut::byExact("test::before"));
    bool beforeCalled = false;
    aspect.setBefore([&beforeCalled](const JoinPoint& jp) {
        beforeCalled = true;
        QCOMPARE(QString::fromStdString(jp.methodName()), QString("test::before"));
    });
    AspectWeaver::instance().registerAspect(std::move(aspect));

    bool targetCalled = false;
    auto result = AspectWeaver::instance().weave(
        "test::before",
        [&targetCalled](JoinPoint&) -> JoinPoint::ReturnType {
            targetCalled = true;
            return 42;
        });
    QVERIFY(beforeCalled);
    QVERIFY(targetCalled);
    QVERIFY(result.has_value());
    QCOMPARE(std::any_cast<int>(result), 42);
}

void TestAop::testAfterAdvice() {
    Aspect aspect("after_test");
    aspect.setPointcut(Pointcut::byExact("test::after"));
    bool afterCalled = false;
    aspect.setAfter([&afterCalled](const JoinPoint&) {
        afterCalled = true;
    });
    AspectWeaver::instance().registerAspect(std::move(aspect));

    AspectWeaver::instance().weave(
        "test::after",
        [](JoinPoint&) -> JoinPoint::ReturnType { return 0; });
    QVERIFY(afterCalled);
}

void TestAop::testAfterReturningAdvice() {
    Aspect aspect("returning_test");
    aspect.setPointcut(Pointcut::byExact("test::returning"));
    bool returningCalled = false;
    aspect.setAfterReturning([&returningCalled](const JoinPoint& jp) {
        returningCalled = true;
        QVERIFY(!jp.hasException());
    });
    AspectWeaver::instance().registerAspect(std::move(aspect));

    AspectWeaver::instance().weave(
        "test::returning",
        [](JoinPoint&) -> JoinPoint::ReturnType { return 100; });
    QVERIFY(returningCalled);
}

void TestAop::testAfterThrowingAdvice() {
    Aspect aspect("throwing_test");
    aspect.setPointcut(Pointcut::byExact("test::throwing"));
    bool throwingCalled = false;
    aspect.setAfterThrowing([&throwingCalled](const JoinPoint& jp) {
        throwingCalled = true;
        QVERIFY(jp.hasException());
    });
    // After 也应在异常时触发
    bool afterCalled = false;
    aspect.setAfter([&afterCalled](const JoinPoint&) {
        afterCalled = true;
    });
    AspectWeaver::instance().registerAspect(std::move(aspect));

    QVERIFY_EXCEPTION_THROWN(
        AspectWeaver::instance().weave(
            "test::throwing",
            [](JoinPoint&) -> JoinPoint::ReturnType {
                throw std::runtime_error("target error");
            }),
        std::runtime_error);
    QVERIFY(throwingCalled);
    QVERIFY(afterCalled);
}

// 验证异常类型保留: 非 runtime_error 类型应原样抛出(通过 exception_ptr 实现)
void TestAop::testExceptionTypePreservation() {
    Aspect aspect("type_preservation");
    aspect.setPointcut(Pointcut::byExact("test::type"));
    bool throwingCalled = false;
    std::string caughtType;
    aspect.setAfterThrowing([&throwingCalled, &caughtType](const JoinPoint& jp) {
        throwingCalled = true;
        QVERIFY(jp.hasException());
        caughtType = jp.exceptionMessage();
    });
    AspectWeaver::instance().registerAspect(std::move(aspect));

    // 抛 std::invalid_argument(非 runtime_error),验证类型保留
    bool caughtCorrectType = false;
    try {
        AspectWeaver::instance().weave(
            "test::type",
            [](JoinPoint&) -> JoinPoint::ReturnType {
                throw std::invalid_argument("invalid arg");
            });
    } catch (const std::invalid_argument&) {
        caughtCorrectType = true;
    } catch (const std::exception& e) {
        QFAIL(QString("Expected std::invalid_argument but caught std::exception: %1")
            .arg(QString::fromStdString(e.what())).toUtf8().constData());
    }

    QVERIFY(caughtCorrectType);
    QVERIFY(throwingCalled);
    QVERIFY(caughtType.find("invalid arg") != std::string::npos);
}

// ============================================================================
// Around advice 测试
// ============================================================================
void TestAop::testAroundAdvice() {
    Aspect aspect("around_test");
    aspect.setPointcut(Pointcut::byExact("test::around"));
    bool aroundCalled = false;
    bool targetCalled = false;
    aspect.setAround([&aroundCalled, &targetCalled](JoinPoint& jp,
                                                      const JoinPoint::ProceedFunc& proceed) -> JoinPoint::ReturnType {
        aroundCalled = true;
        // Around 前半在目标前执行
        if (targetCalled) { throw std::runtime_error("target should not be called before proceed"); }
        auto result = proceed(jp);
        // Around 后半在目标后执行
        if (!targetCalled) { throw std::runtime_error("target should be called after proceed"); }
        return result;
    });
    AspectWeaver::instance().registerAspect(std::move(aspect));

    auto result = AspectWeaver::instance().weave(
        "test::around",
        [&targetCalled](JoinPoint&) -> JoinPoint::ReturnType {
            targetCalled = true;
            return 99;
        });
    QVERIFY(aroundCalled);
    QVERIFY(targetCalled);
    QCOMPARE(std::any_cast<int>(result), 99);
}

// Around advice 可跳过目标方法(不调用 proceed)
void TestAop::testAroundAdviceCanSkipTarget() {
    Aspect aspect("around_skip");
    aspect.setPointcut(Pointcut::byExact("test::skip"));
    bool targetCalled = false;
    aspect.setAround([&targetCalled](JoinPoint&,
                                       const JoinPoint::ProceedFunc&) -> JoinPoint::ReturnType {
        // 不调用 proceed,直接返回缓存值
        return 777;  // 跳过目标
    });
    AspectWeaver::instance().registerAspect(std::move(aspect));

    auto result = AspectWeaver::instance().weave(
        "test::skip",
        [&targetCalled](JoinPoint&) -> JoinPoint::ReturnType {
            targetCalled = true;
            return 0;
        });
    QVERIFY(!targetCalled);  // 目标被跳过
    QCOMPARE(std::any_cast<int>(result), 777);
}

// ============================================================================
// 多切面组合织入顺序测试
// ============================================================================
void TestAop::testMultipleAspectsOrder() {
    std::vector<std::string> order;

    Aspect aspect1("first");
    aspect1.setPointcut(Pointcut::byPrefix("multi::"));
    aspect1.setBefore([&order](const JoinPoint&) { order.push_back("before1"); });
    aspect1.setAfter([&order](const JoinPoint&) { order.push_back("after1"); });
    AspectWeaver::instance().registerAspect(std::move(aspect1));

    Aspect aspect2("second");
    aspect2.setPointcut(Pointcut::byPrefix("multi::"));
    aspect2.setBefore([&order](const JoinPoint&) { order.push_back("before2"); });
    aspect2.setAfter([&order](const JoinPoint&) { order.push_back("after2"); });
    AspectWeaver::instance().registerAspect(std::move(aspect2));

    AspectWeaver::instance().weave(
        "multi::test",
        [&order](JoinPoint&) -> JoinPoint::ReturnType {
            order.push_back("target");
            return 0;
        });

    // Before 按注册顺序,After 也按注册顺序(对标 SpringBoot)
    QCOMPARE(static_cast<int>(order.size()), 5);
    QCOMPARE(QString::fromStdString(order[0]), QString("before1"));
    QCOMPARE(QString::fromStdString(order[1]), QString("before2"));
    QCOMPARE(QString::fromStdString(order[2]), QString("target"));
    QCOMPARE(QString::fromStdString(order[3]), QString("after1"));
    QCOMPARE(QString::fromStdString(order[4]), QString("after2"));
}

// ============================================================================
// 注册/注销测试
// ============================================================================
void TestAop::testRegisterUnregister() {
    AspectWeaver::instance().clear();
    QCOMPARE(static_cast<int>(AspectWeaver::instance().aspectCount()), 0);

    Aspect aspect("temp");
    aspect.setPointcut(Pointcut::byPrefix("temp::"));
    aspect.setBefore([](const JoinPoint&) {});
    AspectWeaver::instance().registerAspect(std::move(aspect));
    QCOMPARE(static_cast<int>(AspectWeaver::instance().aspectCount()), 1);

    AspectWeaver::instance().unregisterAspect("temp");
    QCOMPARE(static_cast<int>(AspectWeaver::instance().aspectCount()), 0);
}

QTEST_GUILESS_MAIN(TestAop)
#include "test_aop.moc"
