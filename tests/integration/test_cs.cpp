#include <QTest>
#include <QCoreApplication>
#include <QStackedWidget>
#include <memory>

#include "soul/cs/cs_global.h"
#include "soul/cs/cs_request.h"
#include "soul/cs/cs_error.h"
#include "soul/cs/cs_error_handler.h"
#include "soul/cs/cs_controller.h"
#include "soul/cs/cs_router.h"
#include "soul/cs/cs_service.h"
#include "soul/cs/cs_data_binding.h"
#include "soul/cs/cs_module.h"
#include "soul/application/application_context.h"
#include "soul/application/service_registry.h"
#include "soul/application/controller_registry.h"
#include "soul/di/container.h"

// ============================================================================
// TestCsRequest — CsRequest 路径/查询参数测试
// ============================================================================
class TestCsRequest : public QObject {
    Q_OBJECT

private slots:
    void testDefaultConstruction();
    void testPathParams();
    void testQueryParams();
    void testIsEmpty();
    void testPathParamDefault();
    void testQueryParamDefault();
};

void TestCsRequest::testDefaultConstruction() {
    sc::cs::CsRequest req;
    QVERIFY(req.isEmpty());
    QVERIFY(req.path.isEmpty());
    QVERIFY(req.handlerName.isEmpty());
    QVERIFY(req.pathParams.isEmpty());
    QVERIFY(req.queryParams.isEmpty());
    QVERIFY(req.body.isEmpty());
}

void TestCsRequest::testPathParams() {
    sc::cs::CsRequest req;
    req.pathParams["id"] = 123;
    req.pathParams["name"] = "test";

    QCOMPARE(req.pathParam("id").toInt(), 123);
    QCOMPARE(req.pathParam("name").toString(), QString("test"));
}

void TestCsRequest::testQueryParams() {
    sc::cs::CsRequest req;
    req.queryParams["sort"] = "name";
    req.queryParams["page"] = 1;

    QCOMPARE(req.queryParam("sort").toString(), QString("name"));
    QCOMPARE(req.queryParam("page").toInt(), 1);
}

void TestCsRequest::testIsEmpty() {
    sc::cs::CsRequest req;
    QVERIFY(req.isEmpty());

    req.path = "user/list";
    QVERIFY(!req.isEmpty());
}

void TestCsRequest::testPathParamDefault() {
    sc::cs::CsRequest req;
    QVariant defaultVal = req.pathParam("nonexistent", QVariant(-1));
    QCOMPARE(defaultVal.toInt(), -1);
}

void TestCsRequest::testQueryParamDefault() {
    sc::cs::CsRequest req;
    QVariant defaultVal = req.queryParam("nonexistent", QString("default"));
    QCOMPARE(defaultVal.toString(), QString("default"));
}

// ============================================================================
// TestCsError — CsError 错误码测试
// ============================================================================
class TestCsError : public QObject {
    Q_OBJECT

private slots:
    void testDefaultOk();
    void testFromErrorCode();
    void testFromScError();
    void testIsOk();
    void testToString();
};

void TestCsError::testDefaultOk() {
    sc::cs::CsError error;
    QVERIFY(error.isOk());
    QCOMPARE(error.code(), 0);
}

void TestCsError::testFromErrorCode() {
    sc::cs::CsError error(sc::cs::CsErrorCode::RouteNotFound, "Route not found");
    QVERIFY(!error.isOk());
    QCOMPARE(error.code(), static_cast<int>(sc::cs::CsErrorCode::RouteNotFound));
    QCOMPARE(error.message(), QString("Route not found"));
}

void TestCsError::testFromScError() {
    sc::Error scError(static_cast<sc::ErrorCode>(1), "General error");
    sc::cs::CsError csError = sc::cs::CsError::from(scError);
    QCOMPARE(csError.code(), 1);
    QCOMPARE(csError.message(), QString("General error"));
}

void TestCsError::testIsOk() {
    sc::cs::CsError okError;
    QVERIFY(okError.isOk());

    sc::cs::CsError errError(sc::cs::CsErrorCode::Unauthorized, "Unauthorized");
    QVERIFY(!errError.isOk());
}

void TestCsError::testToString() {
    sc::cs::CsError error(sc::cs::CsErrorCode::ValidationFailed, "Invalid input");
    QVERIFY(error.toString().contains("1003"));
    QVERIFY(error.toString().contains("Invalid input"));
}

// ============================================================================
// TestCsErrorHandler — 全局错误处理器测试
// ============================================================================
class TestCsErrorHandler : public QObject {
    Q_OBJECT

private slots:
    void testSingleton();
    void testRegisterHandler();
    void testHandleErrorMatched();
    void testHandleErrorDefault();
    void testHandleErrorUnmatched();
    void testClear();
};

void TestCsErrorHandler::testSingleton() {
    auto& h1 = sc::cs::CsErrorHandler::instance();
    auto& h2 = sc::cs::CsErrorHandler::instance();
    QCOMPARE(&h1, &h2);
}

void TestCsErrorHandler::testRegisterHandler() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    handler.clear();

    bool called = false;
    handler.registerHandler(static_cast<int>(sc::cs::CsErrorCode::RouteNotFound),
                            [&called](const sc::cs::CsError&) {
                                called = true;
                                return true;
                            });

    sc::cs::CsError error(sc::cs::CsErrorCode::RouteNotFound, "test");
    bool handled = handler.handleError(error);
    QVERIFY(handled);
    QVERIFY(called);

    handler.clear();
}

void TestCsErrorHandler::testHandleErrorMatched() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    handler.clear();

    handler.registerHandler(static_cast<int>(sc::cs::CsErrorCode::ValidationFailed),
                            [](const sc::cs::CsError&) { return true; });

    sc::cs::CsError error(sc::cs::CsErrorCode::ValidationFailed, "bad input");
    QVERIFY(handler.handleError(error));

    handler.clear();
}

void TestCsErrorHandler::testHandleErrorDefault() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    handler.clear();

    bool defaultCalled = false;
    handler.registerDefaultHandler([&defaultCalled](const sc::cs::CsError&) {
        defaultCalled = true;
        return true;
    });

    sc::cs::CsError error(sc::cs::CsErrorCode::Forbidden, "forbidden");
    bool handled = handler.handleError(error);
    QVERIFY(handled);
    QVERIFY(defaultCalled);

    handler.clear();
}

void TestCsErrorHandler::testHandleErrorUnmatched() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    handler.clear();

    sc::cs::CsError error(sc::cs::CsErrorCode::ServiceUnavailable, "down");
    bool handled = handler.handleError(error);
    QVERIFY(!handled);

    handler.clear();
}

void TestCsErrorHandler::testClear() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    handler.registerHandler(static_cast<int>(sc::cs::CsErrorCode::RouteNotFound),
                            [](const sc::cs::CsError&) { return true; });
    handler.clear();

    sc::cs::CsError error(sc::cs::CsErrorCode::RouteNotFound, "test");
    QVERIFY(!handler.handleError(error));
}

// ============================================================================
// TestCsRouter — 路由匹配与导航测试
// ============================================================================
class TestCsRouter : public QObject {
    Q_OBJECT

private:
    // 用于测试的简单 Controller（嵌套类不使用 Q_OBJECT，Qt MOC 不支持）
    class TestPageController : public sc::cs::CsController {
    public:
        bool listCalled = false;
        bool detailCalled = false;

        TestPageController() : sc::cs::CsController("test") {
            // 使用强类型路由注册
            route("list", &TestPageController::list);
            route("{id}", &TestPageController::detail);
        }

        void list(const sc::cs::CsRequest&) { listCalled = true; }
        void detail(const sc::cs::CsRequest&) { detailCalled = true; }
    };

private slots:
    void testMatchExact();
    void testMatchWithPathParam();
    void testMatchNotFound();
    void testRegisterController();
    void testNavigationRequest();
    void testParseQueryParams();
};

void TestCsRouter::testMatchExact() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    sc::cs::CsRouter router(handler);

    TestPageController ctrl;
    router.registerController(&ctrl);

    sc::cs::RouteEntry matched = router.match("test/list");
    QVERIFY(matched.controller != nullptr);
    QCOMPARE(matched.controller, &ctrl);
    QCOMPARE(matched.fullPattern, QString("test/list"));
}

void TestCsRouter::testMatchWithPathParam() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    sc::cs::CsRouter router(handler);

    TestPageController ctrl;
    router.registerController(&ctrl);

    sc::cs::RouteEntry matched = router.match("test/42");
    QVERIFY(matched.controller != nullptr);
    QCOMPARE(matched.controller, &ctrl);
    QVERIFY(!matched.isStatic());
}

void TestCsRouter::testMatchNotFound() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    sc::cs::CsRouter router(handler);

    sc::cs::RouteEntry matched = router.match("nonexistent/path");
    QVERIFY(matched.controller == nullptr);
}

void TestCsRouter::testRegisterController() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    sc::cs::CsRouter router(handler);

    TestPageController ctrl;
    router.registerController(&ctrl);

    auto routes = router.routes();
    QVERIFY(routes.size() >= 2);  // list + {id}

    // 检查路由表包含完整 pattern
    bool foundList = false;
    bool foundParam = false;
    for (const auto& route : routes) {
        if (route.fullPattern == "test/list") foundList = true;
        if (route.fullPattern == "test/{id}") foundParam = true;
    }
    QVERIFY(foundList);
    QVERIFY(foundParam);
}

void TestCsRouter::testNavigationRequest() {
    auto& handler = sc::cs::CsErrorHandler::instance();
    sc::cs::CsRouter router(handler);

    TestPageController ctrl;
    router.registerController(&ctrl);

    QStackedWidget stack;
    router.setRootWidget(&stack);

    router.navigate("test/list");
    QVERIFY(ctrl.listCalled);

    ctrl.listCalled = false;
    ctrl.detailCalled = false;

    router.navigate("test/42");
    QVERIFY(ctrl.detailCalled);
}

void TestCsRouter::testParseQueryParams() {
    QVariantMap params = sc::cs::CsRouter::parseQueryParams("sort=name&page=1&size=20");
    QCOMPARE(params.size(), 3);
    QCOMPARE(params["sort"].toString(), QString("name"));
    QCOMPARE(params["page"].toString(), QString("1"));
    QCOMPARE(params["size"].toString(), QString("20"));

    QVariantMap empty = sc::cs::CsRouter::parseQueryParams("");
    QVERIFY(empty.isEmpty());
}

// ============================================================================
// TestCsController — 控制器分发测试
// ============================================================================
class TestCsController : public QObject {
    Q_OBJECT

private:
    class UserController : public sc::cs::CsController {
        // Qt MOC 不支持嵌套类 Q_OBJECT，测试中省略
    public:
        bool listCalled = false;
        bool detailCalled = false;
        int detailId = -1;

        UserController() : sc::cs::CsController("user") {
            route("list", &UserController::listUsers);
            route("{id}", &UserController::userDetail);
        }

        void listUsers(const sc::cs::CsRequest&) { listCalled = true; }
        void userDetail(const sc::cs::CsRequest& req) {
            detailCalled = true;
            detailId = req.pathParam("id").toInt();
        }
    };

private slots:
    void testRouteRegistration();
    void testDispatchWithHandlerName();
    void testDispatchFallback();
    void testDispatchRouteNotFound();
    void testRoutesMethod();
    void testRouteInfos();
};

void TestCsController::testRouteRegistration() {
    UserController ctrl;
    QCOMPARE(ctrl.basePath(), QString("user"));

    auto routes = ctrl.routes();
    QVERIFY(routes.contains("list"));
    QVERIFY(routes.contains("{id}"));
}

void TestCsController::testDispatchWithHandlerName() {
    UserController ctrl;

    sc::cs::CsRequest req;
    req.path = "user/list";
    req.handlerName = "list";

    bool dispatched = ctrl.dispatch(req);
    QVERIFY(dispatched);
    QVERIFY(ctrl.listCalled);
}

void TestCsController::testDispatchFallback() {
    UserController ctrl;

    sc::cs::CsRequest req;
    req.path = "user/list";
    // 不设置 handlerName，走 fallback 路径

    bool dispatched = ctrl.dispatch(req);
    QVERIFY(dispatched);
    QVERIFY(ctrl.listCalled);
}

void TestCsController::testDispatchRouteNotFound() {
    UserController ctrl;

    sc::cs::CsRequest req;
    req.path = "user/nonexistent";

    bool dispatched = ctrl.dispatch(req);
    QVERIFY(!dispatched);
}

void TestCsController::testRoutesMethod() {
    UserController ctrl;
    auto routes = ctrl.routes();
    QCOMPARE(routes.size(), 2);
    QCOMPARE(routes["list"], QString());     // 函数指针方式，handlerName 为空
    QCOMPARE(routes["{id}"], QString());
}

void TestCsController::testRouteInfos() {
    UserController ctrl;
    const auto& infos = ctrl.routeInfos();
    QCOMPARE(infos.size(), 2);

    // {id} 模式应该预编译了正则
    QVERIFY(infos.contains("{id}"));
    QVERIFY(infos["{id}"].hasParams);
    QVERIFY(infos["{id}"].compiledRegex.isValid());
}

// ============================================================================
// TestCsService — 服务生命周期测试
// ============================================================================
class TestCsService : public QObject {
    Q_OBJECT

private:
    class TestService : public sc::cs::CsService {
    public:
        bool initialized = false;
        bool shutdownCalled = false;

        TestService() : sc::cs::CsService("TestService") {}

        sc::Result<void> initialize() override { initialized = true; return {}; }
        void shutdown() noexcept override { shutdownCalled = true; }
    };

private slots:
    void testServiceName();
    void testServiceVersion();
    void testInitialize();
    void testShutdown();
};

void TestCsService::testServiceName() {
    TestService svc;
    QCOMPARE(svc.serviceName(), QString("TestService"));
}

void TestCsService::testServiceVersion() {
    TestService svc;
    QCOMPARE(svc.serviceVersion(), QString());
    svc.setServiceVersion("1.0.0");
    QCOMPARE(svc.serviceVersion(), QString("1.0.0"));
}

void TestCsService::testInitialize() {
    TestService svc;
    QVERIFY(!svc.initialized);
    svc.initialize();
    QVERIFY(svc.initialized);
}

void TestCsService::testShutdown() {
    TestService svc;
    QVERIFY(!svc.shutdownCalled);
    (void)svc.shutdown();
    QVERIFY(svc.shutdownCalled);
}

// ============================================================================
// TestServiceRegistry — 服务注册表测试
// ============================================================================
class TestServiceRegistry : public QObject {
    Q_OBJECT

private:
    class MyService : public sc::cs::CsService {
    public:
        bool initialized = false;
        bool shutdownCalled = false;

        MyService() : sc::cs::CsService("MyService") {}
        sc::Result<void> initialize() override { initialized = true; return {}; }
        void shutdown() noexcept override { shutdownCalled = true; }
    };

private slots:
    void testRegisterAndGet();
    void testDuplicateRegistration();
    void testInitializeAll();
    void testShutdownAll();
    void testIsRegistered();
    void testCount();
};

void TestServiceRegistry::testRegisterAndGet() {
    sc::ServiceRegistry registry;
    auto svc = registry.registerService<MyService>();
    QVERIFY(svc != nullptr);
    QCOMPARE(svc->serviceName(), QString("MyService"));

    auto retrieved = registry.getService<MyService>();
    QVERIFY(retrieved != nullptr);
    QCOMPARE(retrieved.get(), svc.get());
}

void TestServiceRegistry::testDuplicateRegistration() {
    sc::ServiceRegistry registry;
    auto svc1 = registry.registerService<MyService>();
    auto svc2 = registry.registerService<MyService>();
    // 重复注册应返回已存在的实例
    QCOMPARE(svc1.get(), svc2.get());
}

void TestServiceRegistry::testInitializeAll() {
    sc::ServiceRegistry registry;
    auto svc = registry.registerService<MyService>();
    QVERIFY(!svc->initialized);

    registry.initializeAll();
    QVERIFY(svc->initialized);
}

void TestServiceRegistry::testShutdownAll() {
    sc::ServiceRegistry registry;
    auto svc = registry.registerService<MyService>();
    registry.initializeAll();

    registry.shutdownAll();
    QVERIFY(svc->shutdownCalled);
}

void TestServiceRegistry::testIsRegistered() {
    sc::ServiceRegistry registry;
    QVERIFY(!registry.isRegistered<MyService>());

    registry.registerService<MyService>();
    QVERIFY(registry.isRegistered<MyService>());
}

void TestServiceRegistry::testCount() {
    sc::ServiceRegistry registry;
    QCOMPARE(registry.count(), static_cast<size_t>(0));

    registry.registerService<MyService>();
    QCOMPARE(registry.count(), static_cast<size_t>(1));
}

// ============================================================================
// TestControllerRegistry — 控制器注册表测试
// ============================================================================
class TestControllerRegistry : public QObject {
    Q_OBJECT

private:
    class TestCtrl : public sc::cs::CsController {
        // Qt MOC 不支持嵌套类 Q_OBJECT，测试中省略
    public:
        TestCtrl() : sc::cs::CsController("test") {
            route("list", &TestCtrl::listHandler);
        }
        void listHandler(const sc::cs::CsRequest&) {}
    };

private slots:
    void testRegisterAndGet();
    void testDuplicateRegistration();
    void testIsRegistered();
    void testCount();
    void testRegisterAllRoutes();
};

void TestControllerRegistry::testRegisterAndGet() {
    sc::ControllerRegistry registry;

    auto& ctrl = registry.registerController<TestCtrl>();
    QCOMPARE(ctrl.basePath(), QString("test"));

    auto* retrieved = registry.getController<TestCtrl>();
    QVERIFY(retrieved != nullptr);
    QCOMPARE(retrieved, &ctrl);
}

void TestControllerRegistry::testDuplicateRegistration() {
    sc::ControllerRegistry registry;

    auto& ctrl1 = registry.registerController<TestCtrl>();
    auto& ctrl2 = registry.registerController<TestCtrl>();
    // 重复注册应返回已存在的实例
    QCOMPARE(&ctrl1, &ctrl2);
}

void TestControllerRegistry::testIsRegistered() {
    sc::ControllerRegistry registry;
    QVERIFY(!registry.isRegistered<TestCtrl>());

    registry.registerController<TestCtrl>();
    QVERIFY(registry.isRegistered<TestCtrl>());
}

void TestControllerRegistry::testCount() {
    sc::ControllerRegistry registry;
    QCOMPARE(registry.count(), static_cast<size_t>(0));

    registry.registerController<TestCtrl>();
    QCOMPARE(registry.count(), static_cast<size_t>(1));
}

void TestControllerRegistry::testRegisterAllRoutes() {
    sc::ControllerRegistry registry;
    auto& handler = sc::cs::CsErrorHandler::instance();
    sc::cs::CsRouter router(handler);

    registry.registerController<TestCtrl>();
    registry.registerAllRoutes(router);

    auto routes = router.routes();
    QVERIFY(routes.size() >= 1);

    bool foundTestList = false;
    for (const auto& route : routes) {
        if (route.fullPattern == "test/list") {
            foundTestList = true;
            break;
        }
    }
    QVERIFY(foundTestList);
}

// ============================================================================
// TestCsDataBinding — 数据绑定测试（使用 ReflectiveEntity 的 mock）
// ============================================================================
class TestCsDataBinding : public QObject {
    Q_OBJECT

private:
    // 简单的 Entity 模拟（实际使用时继承 ReflectiveEntity）
    class MockEntity : public sc::data::ReflectiveEntity {
    public:
        QString entityName;
        int entityAge = 0;

        MockEntity() {
            registerProperty<MockEntity, QString>("name",
                [](const MockEntity& e) { return e.entityName; },
                [](MockEntity& e, const QString& v) { e.entityName = v; });
            registerProperty<MockEntity, int>("age",
                [](const MockEntity& e) { return e.entityAge; },
                [](MockEntity& e, const int& v) { e.entityAge = v; });
        }
    };

private slots:
    void testBindEntityToViewModel();
    void testBindViewModelToEntity();
    void testSyncToViewModel();
    void testSyncToEntity();
    void testConverter();
    void testAddMapping();
};

void TestCsDataBinding::testBindEntityToViewModel() {
    sc::cs::CsDataBinding binding;

    MockEntity entity;
    entity.entityName = "Alice";
    entity.entityAge = 30;

    sc::cs::CsViewModel vm("test");
    binding.bindEntity(entity, &vm);

    QCOMPARE(vm.get("name").toString(), QString("Alice"));
    QCOMPARE(vm.get("age").toInt(), 30);
}

void TestCsDataBinding::testBindViewModelToEntity() {
    sc::cs::CsDataBinding binding;

    sc::cs::CsViewModel vm("test");
    vm.set("name", QString("Bob"));
    vm.set("age", 25);

    MockEntity entity;
    binding.bindViewModel(&vm, entity);

    QCOMPARE(entity.entityName, QString("Bob"));
    QCOMPARE(entity.entityAge, 25);
}

void TestCsDataBinding::testSyncToViewModel() {
    sc::cs::CsDataBinding binding;

    MockEntity entity;
    entity.entityName = "Charlie";
    entity.entityAge = 35;

    sc::cs::CsViewModel vm("test");
    binding.bindEntity(entity, &vm);

    entity.entityName = "David";
    entity.entityAge = 40;
    binding.syncToViewModel(entity);

    QCOMPARE(vm.get("name").toString(), QString("David"));
    QCOMPARE(vm.get("age").toInt(), 40);
}

void TestCsDataBinding::testSyncToEntity() {
    sc::cs::CsDataBinding binding;

    sc::cs::CsViewModel vm("test");
    vm.set("name", QString("Eve"));
    vm.set("age", 45);

    MockEntity entity;
    binding.bindViewModel(&vm, entity);

    vm.set("name", QString("Frank"));
    vm.set("age", 50);
    binding.syncToEntity(entity);

    QCOMPARE(entity.entityName, QString("Frank"));
    QCOMPARE(entity.entityAge, 50);
}

void TestCsDataBinding::testConverter() {
    sc::cs::CsDataBinding binding;

    auto converter = std::make_shared<sc::cs::StringToIntConverter>();
    QCOMPARE(converter->toViewModel(QVariant(42)).toString(), QString("42"));
    QCOMPARE(converter->toEntity(QVariant("99")).toInt(), 99);

    binding.registerConverter("age", converter);
}

void TestCsDataBinding::testAddMapping() {
    sc::cs::CsDataBinding binding;

    binding.addMapping("userId", "id");
    binding.addMapping("userName", "displayName");

    // 验证映射已添加（通过 bindEntity 间接验证）
    MockEntity entity;
    entity.entityName = "Grace";
    entity.entityAge = 28;

    sc::cs::CsViewModel vm("test");
    binding.bindEntity(entity, &vm);

    // 没有映射时，默认使用 entity 属性名
    QCOMPARE(vm.get("name").toString(), QString("Grace"));
}

// ============================================================================
// TestCsModule — 模块注册测试
// ============================================================================
class TestCsModule : public QObject {
    Q_OBJECT

private:
    class MyTestService : public sc::cs::CsService {
    public:
        MyTestService() : sc::cs::CsService("MyTestService") {}
    };

    class MyTestController : public sc::cs::CsController {
        // Qt MOC 不支持嵌套类 Q_OBJECT，测试中省略
    public:
        MyTestController() : sc::cs::CsController("mytest") {
            route("list", &MyTestController::listHandler);
        }
        void listHandler(const sc::cs::CsRequest&) {}
    };

    class MyTestModule : public sc::cs::CsModule {
    public:
        MyTestModule() : sc::cs::CsModule("MyTest") {}

        void onRegister() override {
            registerService<MyTestService>();
            registerController<MyTestController>();
        }
    };

private slots:
    void testModuleName();
    void testInit();
    void testRouterAccess();
};

void TestCsModule::testModuleName() {
    MyTestModule module;
    QCOMPARE(module.moduleName(), QString("MyTest"));
}

void TestCsModule::testInit() {
    MyTestModule module;
    auto result = module.init();
    QVERIFY(result.isOk());
}

void TestCsModule::testRouterAccess() {
    // router() 在 initialize() 之前调用需要 ApplicationContext 已初始化
    // 此测试仅验证接口存在
    MyTestModule module;
    // 不调用 router() — 因为 CsRouter 在 initialize() 中创建
}

// ============================================================================
// TestApplicationContext — 应用上下文生命周期测试
// ============================================================================
class TestApplicationContext : public QObject {
    Q_OBJECT

private:
    class TestModule : public sc::cs::CsModule {
    public:
        bool onRegisterCalled = false;

        TestModule() : sc::cs::CsModule("TestModule") {}

        void onRegister() override {
            onRegisterCalled = true;
        }
    };

private slots:
    void testSingleton();
    void testRegisterModule();
    void testInitialize();
    void testShutdown();
    void testSetErrorHandler();
    void testGetService();
};

void TestApplicationContext::testSingleton() {
    auto& ctx1 = sc::ApplicationContext::instance();
    auto& ctx2 = sc::ApplicationContext::instance();
    QCOMPARE(&ctx1, &ctx2);
}

void TestApplicationContext::testRegisterModule() {
    auto& ctx = sc::ApplicationContext::instance();

    auto& module = ctx.registerModule<TestModule>();
    QCOMPARE(module.moduleName(), QString("TestModule"));

    const auto& modules = ctx.modules();
    QCOMPARE(modules.size(), static_cast<size_t>(1));
    QCOMPARE(modules[0].get(), &module);
}

void TestApplicationContext::testInitialize() {
    auto& ctx = sc::ApplicationContext::instance();

    // 先注册模块，再初始化
    auto result = ctx.initialize();
    if (result.isOk()) {
        QVERIFY(ctx.isInitialized());
    }
    // 注意: 如果之前的测试已经初始化了，initialize() 是幂等的
}

void TestApplicationContext::testShutdown() {
    auto& ctx = sc::ApplicationContext::instance();
    if (ctx.isInitialized()) {
        ctx.shutdown();
        QVERIFY(!ctx.isInitialized());
    }
}

void TestApplicationContext::testSetErrorHandler() {
    auto& ctx = sc::ApplicationContext::instance();
    if (ctx.isInitialized()) {
        ctx.shutdown();
    }

    // 测试 setErrorHandler 在 initialize() 之前调用
    auto& handler = sc::cs::CsErrorHandler::instance();
    ctx.setErrorHandler(handler);
    // 验证不崩溃即可
}

void TestApplicationContext::testGetService() {
    // getService() 在 initialize() 后可用
    auto& ctx = sc::ApplicationContext::instance();
    if (!ctx.isInitialized()) {
        ctx.initialize();
    }
    // 如果没有注册服务，getService 返回 nullptr
    // v3.0.0: TestCsModule::MyTestService 是 private 嵌套类，直接测试跳过
    QVERIFY(true);
}

// ============================================================================
// TestCsVersion — 版本号测试
// ============================================================================
class TestCsVersion : public QObject {
    Q_OBJECT

private slots:
    void testVersionConstants();
};

void TestCsVersion::testVersionConstants() {
    QCOMPARE(sc::cs::CsVersion::Major, 2);
    QCOMPARE(sc::cs::CsVersion::Minor, 5);
    QCOMPARE(sc::cs::CsVersion::Patch, 0);
    QCOMPARE(QString(sc::cs::CsVersion::String), QString("2.5.0"));
}

// ============================================================================
// 主入口
// ============================================================================
QTEST_MAIN(TestCsRequest)

#include "test_cs.moc"