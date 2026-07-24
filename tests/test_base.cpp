#include <QTest>
#include <QSignalSpy>
#include <QVariant>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "soul/base/base_object.h"
#include "soul/base/base_manager.h"
#include "soul/base/base_service.h"
#include "soul/data/repository.h"
#include "soul/data/memory_repository.h"
#include "soul/ui/base_view_model.h"
#include "soul/ui/base_widget.h"
#include "soul/ui/base_dialog.h"
#include "soul/ui/theme.h"

using namespace sc;
using sc::ui::BaseWidget;
using sc::ui::BaseDialog;
using sc::ui::BaseViewModel;

// ---------------------------------------------------------------------------
// Helper subclasses for testing protected members and virtual overrides.
// No Q_OBJECT needed: they only inherit signals/slots and add plain methods.
// ---------------------------------------------------------------------------

// Tracks whether init() was called more than once (edge-case double init).
class TestManager : public BaseManager {
public:
    explicit TestManager(QObject* parent = nullptr) : BaseManager(parent) {}

    bool init() override {
        if (isInitialized()) {
            m_initCalledTwice = true;
        }
        return BaseManager::init();
    }

    void shutdown() override {
        BaseManager::shutdown();
    }

    bool initCalledTwice() const { return m_initCalledTwice; }

private:
    bool m_initCalledTwice = false;
};

// Tracks whether start() was called more than once (edge-case double start).
class TestService : public BaseService {
public:
    explicit TestService(QObject* parent = nullptr) : BaseService(parent) {}

    bool start() override {
        if (isRunning()) {
            m_startCalledTwice = true;
        }
        return BaseService::start();
    }

    void stop() override {
        BaseService::stop();
    }

    bool startCalledTwice() const { return m_startCalledTwice; }

private:
    bool m_startCalledTwice = false;
};

// Exposes the protected setLoading()/emitError() helpers for testing.
class TestViewModel : public sc::ui::BaseViewModel {
public:
    using sc::ui::BaseViewModel::setLoading;
    using sc::ui::BaseViewModel::emitError;
};

// Concrete repository implementation used to verify the data::IRepository
// contract and the default method behaviour provided by the interface.
class InMemoryRepository : public sc::data::IRepository<std::string, std::string> {
public:
    Result<std::string> findById(const std::string& id) override {
        auto it = m_data.find(id);
        if (it != m_data.end()) {
            return Result<std::string>::ok(it->second);
        }
        return Result<std::string>::err(Error(ErrorCode::NotFound, "Not found: " + id));
    }

    Result<std::vector<std::string>> findAll() override {
        std::vector<std::string> items;
        items.reserve(m_data.size());
        for (const auto& entry : m_data) {
            items.push_back(entry.second);
        }
        return Result<std::vector<std::string>>::ok(std::move(items));
    }

    Result<std::string> save(const std::string& entity) override {
        m_data[entity] = entity;
        return Result<std::string>::ok(entity);
    }

    Result<void> removeById(const std::string& id) override {
        auto it = m_data.find(id);
        if (it == m_data.end()) {
            return Result<void>::err(Error(ErrorCode::NotFound, "Not found: " + id));
        }
        m_data.erase(it);
        return Result<void>::ok();
    }

    Result<int> count() override {
        return Result<int>::ok(static_cast<int>(m_data.size()));
    }

private:
    std::map<std::string, std::string> m_data;
};

// ---------------------------------------------------------------------------
// Main test fixture
// ---------------------------------------------------------------------------

class TestBase : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // BaseObject tests
    void testBaseObjectDefaultConstructor();
    void testBaseObjectWithDebugName();
    void testBaseObjectDebugNameFallbackToObjectName();
    void testBaseObjectSetDebugName();
    void testBaseObjectDebugNameOverridesObjectName();
    void testBaseObjectPathNoParent();
    void testBaseObjectPathWithBaseObjectParent();
    void testBaseObjectPathWithQObjectParent();
    void testBaseObjectPathNestedHierarchy();
    void testBaseObjectParentOwnership();

    // BaseManager tests
    void testBaseManagerDefaultNotInitialized();
    void testBaseManagerInitReturnsTrue();
    void testBaseManagerShutdownClearsFlag();
    void testBaseManagerDoubleInit();
    void testBaseManagerShutdownWithoutInit();
    void testBaseManagerReinitAfterShutdown();
    void testBaseManagerInheritsBaseObject();
    void testBaseManagerSubclassVirtualOverride();

    // BaseService tests
    void testBaseServiceDefaultNotRunning();
    void testBaseServiceStartReturnsTrue();
    void testBaseServiceStopClearsFlag();
    void testBaseServiceDoubleStart();
    void testBaseServiceStopWithoutStart();
    void testBaseServiceRestartAfterStop();
    void testBaseServiceInheritsBaseObject();
    void testBaseServiceSubclassVirtualOverride();

    // data::IRepository default implementation tests
    void testRepositoryExistsByIdDefault();
    void testRepositoryCountDefault();
    void testRepositorySaveBatchDefault();
    void testRepositoryRemoveBatchDefault();
    void testInMemoryRepositorySave();
    void testInMemoryRepositoryFindById();
    void testInMemoryRepositoryFindByIdNotFound();
    void testInMemoryRepositoryFindAll();
    void testInMemoryRepositoryFindAllEmpty();
    void testInMemoryRepositoryRemove();
    void testInMemoryRepositoryRemoveMissing();
    void testInMemoryRepositorySaveUpsert();
    void testInMemoryRepositoryPolymorphism();

    // BaseViewModel tests
    void testViewModelGetMissingReturnsInvalidVariant();
    void testViewModelGetMissingReturnsProvidedDefault();
    void testViewModelSetAndGet();
    void testViewModelSetEmitsPropertyChange();
    void testViewModelSetSameValueDoesNotEmit();
    void testViewModelGetValueTemplateMissing();
    void testViewModelGetValueTemplate();
    void testViewModelSetValueTemplateEmits();
    void testViewModelSetValueTemplateSameValueNoEmit();
    void testViewModelSetLoadingEmitsSignal();
    void testViewModelSetLoadingSameValueNoEmit();
    void testViewModelEmitErrorEmitsSignal();
    void testViewModelMultipleProperties();
    void testViewModelTypedValueRoundtrip();

    // BaseWidget / BaseDialog tests
    void testBaseWidgetConstruction();
    void testBaseWidgetAppliesStyleSheet();
    void testBaseDialogConstruction();
    void testBaseDialogSetTitle();
    void testBaseDialogAddButton();
    void testBaseDialogAddMultipleButtons();
};

// ---------------------------------------------------------------------------
// Test case setup / teardown
// ---------------------------------------------------------------------------

void TestBase::initTestCase() {
    // The Theme singleton's Style is default-constructed with valid colours,
    // so BaseWidget/BaseDialog can be built safely without calling Theme::init().
    // We still ensure the singleton exists here for determinism.
    QVERIFY(&Theme::instance() != nullptr);
}

void TestBase::cleanupTestCase() {
    // No global state to clean up.
}

// ---------------------------------------------------------------------------
// BaseObject tests
// ---------------------------------------------------------------------------

void TestBase::testBaseObjectDefaultConstructor() {
    BaseObject obj;
    QVERIFY(obj.debugName().isEmpty());
    QCOMPARE(obj.objectPath(), QString());
}

void TestBase::testBaseObjectWithDebugName() {
    BaseObject obj(QStringLiteral("TestObject"));
    QCOMPARE(obj.debugName(), QStringLiteral("TestObject"));
}

void TestBase::testBaseObjectDebugNameFallbackToObjectName() {
    BaseObject obj;
    obj.setObjectName(QStringLiteral("ObjectName"));
    // When m_debugName is empty, debugName() falls back to objectName().
    QCOMPARE(obj.debugName(), QStringLiteral("ObjectName"));
}

void TestBase::testBaseObjectSetDebugName() {
    BaseObject obj;
    obj.setDebugName(QStringLiteral("Debug"));
    QCOMPARE(obj.debugName(), QStringLiteral("Debug"));

    obj.setDebugName(QStringLiteral("NewDebug"));
    QCOMPARE(obj.debugName(), QStringLiteral("NewDebug"));
}

void TestBase::testBaseObjectDebugNameOverridesObjectName() {
    BaseObject obj;
    obj.setObjectName(QStringLiteral("ObjectName"));
    obj.setDebugName(QStringLiteral("Debug"));
    // debugName() should return the explicit debug name over objectName().
    QCOMPARE(obj.debugName(), QStringLiteral("Debug"));
}

void TestBase::testBaseObjectPathNoParent() {
    BaseObject obj(QStringLiteral("Root"));
    QCOMPARE(obj.objectPath(), QStringLiteral("Root"));
}

void TestBase::testBaseObjectPathWithBaseObjectParent() {
    BaseObject parent(QStringLiteral("Parent"));
    BaseObject* child = new BaseObject(QStringLiteral("Child"), &parent);
    QCOMPARE(child->objectPath(), QStringLiteral("Parent/Child"));
    // child is parented to parent; deleting parent deletes child.
}

void TestBase::testBaseObjectPathWithQObjectParent() {
    QObject qparent;
    qparent.setObjectName(QStringLiteral("QParent"));
    BaseObject* child = new BaseObject(QStringLiteral("Child"), &qparent);
    // When the parent is a plain QObject (not BaseObject), the path uses
    // objectName() of the parent.
    QCOMPARE(child->objectPath(), QStringLiteral("QParent/Child"));
    delete child;
}

void TestBase::testBaseObjectPathNestedHierarchy() {
    BaseObject root(QStringLiteral("Root"));
    BaseObject* middle = new BaseObject(QStringLiteral("Middle"), &root);
    BaseObject* leaf = new BaseObject(QStringLiteral("Leaf"), middle);
    QCOMPARE(leaf->objectPath(), QStringLiteral("Root/Middle/Leaf"));
    // Qt parent-child ownership chain cleans up middle and leaf.
}

void TestBase::testBaseObjectParentOwnership() {
    QObject* parent = new QObject();
    size_t initialCount = parent->children().size();
    BaseObject* child = new BaseObject(QStringLiteral("Child"), parent);
    QCOMPARE(static_cast<size_t>(parent->children().size()), initialCount + 1);
    QVERIFY(parent->children().contains(child));
    delete parent; // should delete child as well
}

// ---------------------------------------------------------------------------
// BaseManager tests
// ---------------------------------------------------------------------------

void TestBase::testBaseManagerDefaultNotInitialized() {
    BaseManager manager;
    QVERIFY(!manager.isInitialized());
}

void TestBase::testBaseManagerInitReturnsTrue() {
    BaseManager manager;
    bool result = manager.init();
    QVERIFY(result);
    QVERIFY(manager.isInitialized());
}

void TestBase::testBaseManagerShutdownClearsFlag() {
    BaseManager manager;
    manager.init();
    QVERIFY(manager.isInitialized());
    manager.shutdown();
    QVERIFY(!manager.isInitialized());
}

void TestBase::testBaseManagerDoubleInit() {
    BaseManager manager;
    QVERIFY(manager.init());
    QVERIFY(manager.isInitialized());
    // Calling init() again should remain successful and keep the flag set.
    QVERIFY(manager.init());
    QVERIFY(manager.isInitialized());
}

void TestBase::testBaseManagerShutdownWithoutInit() {
    BaseManager manager;
    QVERIFY(!manager.isInitialized());
    // Shutting down before init is a no-op; flag stays false.
    manager.shutdown();
    QVERIFY(!manager.isInitialized());
}

void TestBase::testBaseManagerReinitAfterShutdown() {
    BaseManager manager;
    QVERIFY(manager.init());
    QVERIFY(manager.isInitialized());
    manager.shutdown();
    QVERIFY(!manager.isInitialized());
    QVERIFY(manager.init());
    QVERIFY(manager.isInitialized());
}

void TestBase::testBaseManagerInheritsBaseObject() {
    BaseManager manager;
    manager.setDebugName(QStringLiteral("MyManager"));
    QCOMPARE(manager.debugName(), QStringLiteral("MyManager"));
    QCOMPARE(manager.objectPath(), QStringLiteral("MyManager"));

    // Verify inheritance chain: BaseManager is-a BaseObject.
    BaseObject* asBase = &manager;
    QCOMPARE(asBase->debugName(), QStringLiteral("MyManager"));
}

void TestBase::testBaseManagerSubclassVirtualOverride() {
    TestManager manager;
    QVERIFY(!manager.isInitialized());
    QVERIFY(!manager.initCalledTwice());

    QVERIFY(manager.init());
    QVERIFY(manager.isInitialized());
    QVERIFY(!manager.initCalledTwice());

    // Second init triggers the "called twice" flag in the override.
    QVERIFY(manager.init());
    QVERIFY(manager.initCalledTwice());

    manager.shutdown();
    QVERIFY(!manager.isInitialized());
}

// ---------------------------------------------------------------------------
// BaseService tests
// ---------------------------------------------------------------------------

void TestBase::testBaseServiceDefaultNotRunning() {
    BaseService service;
    QVERIFY(!service.isRunning());
}

void TestBase::testBaseServiceStartReturnsTrue() {
    BaseService service;
    bool result = service.start();
    QVERIFY(result);
    QVERIFY(service.isRunning());
}

void TestBase::testBaseServiceStopClearsFlag() {
    BaseService service;
    service.start();
    QVERIFY(service.isRunning());
    service.stop();
    QVERIFY(!service.isRunning());
}

void TestBase::testBaseServiceDoubleStart() {
    BaseService service;
    QVERIFY(service.start());
    QVERIFY(service.isRunning());
    // Starting again keeps the service running and returns true.
    QVERIFY(service.start());
    QVERIFY(service.isRunning());
}

void TestBase::testBaseServiceStopWithoutStart() {
    BaseService service;
    QVERIFY(!service.isRunning());
    // Stopping before starting is a no-op; flag stays false.
    service.stop();
    QVERIFY(!service.isRunning());
}

void TestBase::testBaseServiceRestartAfterStop() {
    BaseService service;
    QVERIFY(service.start());
    QVERIFY(service.isRunning());
    service.stop();
    QVERIFY(!service.isRunning());
    QVERIFY(service.start());
    QVERIFY(service.isRunning());
}

void TestBase::testBaseServiceInheritsBaseObject() {
    BaseService service;
    service.setDebugName(QStringLiteral("MyService"));
    QCOMPARE(service.debugName(), QStringLiteral("MyService"));
    QCOMPARE(service.objectPath(), QStringLiteral("MyService"));

    BaseObject* asBase = &service;
    QCOMPARE(asBase->debugName(), QStringLiteral("MyService"));
}

void TestBase::testBaseServiceSubclassVirtualOverride() {
    TestService service;
    QVERIFY(!service.isRunning());
    QVERIFY(!service.startCalledTwice());

    QVERIFY(service.start());
    QVERIFY(service.isRunning());
    QVERIFY(!service.startCalledTwice());

    // Second start triggers the "called twice" flag in the override.
    QVERIFY(service.start());
    QVERIFY(service.startCalledTwice());

    service.stop();
    QVERIFY(!service.isRunning());
}

// ---------------------------------------------------------------------------
// data::IRepository default implementation tests
// ---------------------------------------------------------------------------

void TestBase::testRepositoryExistsByIdDefault() {
    InMemoryRepository repo;
    // existsById() default returns false when entity is missing (NotFound -> false).
    auto missing = repo.existsById("missing");
    QVERIFY(missing.isOk());
    QCOMPARE(missing.unwrap(), false);

    repo.save("alpha");
    // existsById() returns true when entity is present.
    auto present = repo.existsById("alpha");
    QVERIFY(present.isOk());
    QCOMPARE(present.unwrap(), true);
}

void TestBase::testRepositoryCountDefault() {
    InMemoryRepository repo;
    // count() reflects the number of stored entities.
    QCOMPARE(repo.count().unwrap(), 0);
    repo.save("alpha");
    repo.save("beta");
    QCOMPARE(repo.count().unwrap(), 2);
}

void TestBase::testRepositorySaveBatchDefault() {
    InMemoryRepository repo;
    std::vector<std::string> entities = {"alpha", "beta", "gamma"};
    auto result = repo.saveBatch(entities);
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().size(), static_cast<std::size_t>(3));
    QCOMPARE(repo.count().unwrap(), 3);
}

void TestBase::testRepositoryRemoveBatchDefault() {
    InMemoryRepository repo;
    repo.save("alpha");
    repo.save("beta");
    std::vector<std::string> ids = {"alpha", "beta"};
    auto result = repo.removeBatch(ids);
    QVERIFY(result.isOk());
    QCOMPARE(repo.count().unwrap(), 0);
}

// ---------------------------------------------------------------------------
// InMemoryRepository (concrete CRUD) tests
// ---------------------------------------------------------------------------

void TestBase::testInMemoryRepositorySave() {
    InMemoryRepository repo;
    QCOMPARE(repo.count().unwrap(), 0);
    QVERIFY(repo.save("alpha").isOk());
    QCOMPARE(repo.count().unwrap(), 1);
}

void TestBase::testInMemoryRepositoryFindById() {
    InMemoryRepository repo;
    repo.save("alpha");
    auto result = repo.findById("alpha");
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap(), std::string("alpha"));
}

void TestBase::testInMemoryRepositoryFindByIdNotFound() {
    InMemoryRepository repo;
    auto result = repo.findById("missing");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), ErrorCode::NotFound);
}

void TestBase::testInMemoryRepositoryFindAll() {
    InMemoryRepository repo;
    repo.save("alpha");
    repo.save("beta");
    repo.save("gamma");

    auto result = repo.findAll();
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().size(), static_cast<std::size_t>(3));
}

void TestBase::testInMemoryRepositoryFindAllEmpty() {
    InMemoryRepository repo;
    auto result = repo.findAll();
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().size(), static_cast<std::size_t>(0));
}

void TestBase::testInMemoryRepositoryRemove() {
    InMemoryRepository repo;
    repo.save("alpha");
    repo.save("beta");
    QCOMPARE(repo.count().unwrap(), 2);

    QVERIFY(repo.removeById("alpha").isOk());
    QCOMPARE(repo.count().unwrap(), 1);
    QVERIFY(repo.findById("alpha").isErr());
    QVERIFY(repo.findById("beta").isOk());
}

void TestBase::testInMemoryRepositoryRemoveMissing() {
    InMemoryRepository repo;
    QVERIFY(repo.removeById("missing").isErr());
}

void TestBase::testInMemoryRepositorySaveUpsert() {
    InMemoryRepository repo;
    repo.save("alpha");
    QCOMPARE(repo.count().unwrap(), 1);
    // Saving again with same key does not add a new entry (upsert).
    repo.save("alpha");
    QCOMPARE(repo.count().unwrap(), 1);
}

void TestBase::testInMemoryRepositoryPolymorphism() {
    InMemoryRepository concrete;
    sc::data::IRepository<std::string, std::string>* iface = &concrete;

    QVERIFY(iface->save("poly").isOk());
    QCOMPARE(concrete.count().unwrap(), 1);

    auto found = iface->findById("poly");
    QVERIFY(found.isOk());
    QCOMPARE(found.unwrap(), std::string("poly"));

    auto all = iface->findAll();
    QVERIFY(all.isOk());
    QCOMPARE(all.unwrap().size(), static_cast<std::size_t>(1));

    QVERIFY(iface->removeById("poly").isOk());
    QCOMPARE(concrete.count().unwrap(), 0);
}

// ---------------------------------------------------------------------------
// BaseViewModel tests
// ---------------------------------------------------------------------------

void TestBase::testViewModelGetMissingReturnsInvalidVariant() {
    TestViewModel vm;
    QVariant value = vm.get("missing");
    QVERIFY(!value.isValid());
}

void TestBase::testViewModelGetMissingReturnsProvidedDefault() {
    TestViewModel vm;
    QVariant value = vm.get("missing", QStringLiteral("default"));
    QCOMPARE(value.toString(), QStringLiteral("default"));
}

void TestBase::testViewModelSetAndGet() {
    TestViewModel vm;
    vm.set("key", QStringLiteral("value"));
    QCOMPARE(vm.get("key").toString(), QStringLiteral("value"));
}

void TestBase::testViewModelSetEmitsPropertyChange() {
    TestViewModel vm;
    QSignalSpy spy(&vm, &BaseViewModel::propertyChanged);
    QVERIFY(spy.isValid());
    vm.set("key", QStringLiteral("value"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("key"));
}

void TestBase::testViewModelSetSameValueDoesNotEmit() {
    TestViewModel vm;
    vm.set("key", QStringLiteral("value"));
    QSignalSpy spy(&vm, &BaseViewModel::propertyChanged);
    QVERIFY(spy.isValid());
    // Setting the same value must not emit propertyChanged.
    vm.set("key", QStringLiteral("value"));
    QCOMPARE(spy.count(), 0);
}

void TestBase::testViewModelGetValueTemplateMissing() {
    TestViewModel vm;
    // Missing key returns the provided default.
    QCOMPARE(vm.getValue<int>("count", 42), 42);
    // Missing key returns default-constructed T when no default given.
    QCOMPARE(vm.getValue<int>("count"), 0);
}

void TestBase::testViewModelGetValueTemplate() {
    TestViewModel vm;
    vm.setValue("count", 100);
    QCOMPARE(vm.getValue<int>("count"), 100);
}

void TestBase::testViewModelSetValueTemplateEmits() {
    TestViewModel vm;
    QSignalSpy spy(&vm, &BaseViewModel::propertyChanged);
    QVERIFY(spy.isValid());
    vm.setValue("count", 5);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("count"));
    QCOMPARE(vm.getValue<int>("count"), 5);
}

void TestBase::testViewModelSetValueTemplateSameValueNoEmit() {
    TestViewModel vm;
    vm.setValue("count", 5);
    QSignalSpy spy(&vm, &BaseViewModel::propertyChanged);
    QVERIFY(spy.isValid());
    vm.setValue("count", 5);
    QCOMPARE(spy.count(), 0);
}

void TestBase::testViewModelSetLoadingEmitsSignal() {
    TestViewModel vm;
    QSignalSpy spy(&vm, &BaseViewModel::loadingChanged);
    QVERIFY(spy.isValid());
    vm.setLoading(true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toBool(), true);
}

void TestBase::testViewModelSetLoadingSameValueNoEmit() {
    TestViewModel vm;
    vm.setLoading(true);
    QSignalSpy spy(&vm, &BaseViewModel::loadingChanged);
    QVERIFY(spy.isValid());
    // Setting the same loading state must not emit.
    vm.setLoading(true);
    QCOMPARE(spy.count(), 0);

    // Toggling back does emit.
    vm.setLoading(false);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toBool(), false);
}

void TestBase::testViewModelEmitErrorEmitsSignal() {
    TestViewModel vm;
    QSignalSpy spy(&vm, &BaseViewModel::errorOccurred);
    QVERIFY(spy.isValid());
    vm.emitError(QStringLiteral("Something went wrong"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("Something went wrong"));
}

void TestBase::testViewModelMultipleProperties() {
    TestViewModel vm;
    vm.set("name", QStringLiteral("Alice"));
    vm.set("age", 30);
    vm.set("active", true);

    QCOMPARE(vm.get("name").toString(), QStringLiteral("Alice"));
    QCOMPARE(vm.get("age").toInt(), 30);
    QCOMPARE(vm.get("active").toBool(), true);

    // Updating one property leaves the others intact.
    vm.set("age", 31);
    QCOMPARE(vm.get("age").toInt(), 31);
    QCOMPARE(vm.get("name").toString(), QStringLiteral("Alice"));
    QCOMPARE(vm.get("active").toBool(), true);
}

void TestBase::testViewModelTypedValueRoundtrip() {
    TestViewModel vm;
    // int
    vm.setValue("intVal", 42);
    QCOMPARE(vm.getValue<int>("intVal"), 42);
    // double
    vm.setValue("doubleVal", 3.14);
    QCOMPARE(vm.getValue<double>("doubleVal"), 3.14);
    // QString
    vm.setValue("stringVal", QStringLiteral("hello"));
    QCOMPARE(vm.getValue<QString>("stringVal"), QStringLiteral("hello"));
    // bool
    vm.setValue("boolVal", true);
    QCOMPARE(vm.getValue<bool>("boolVal"), true);
}

// ---------------------------------------------------------------------------
// BaseWidget / BaseDialog tests
// ---------------------------------------------------------------------------

void TestBase::testBaseWidgetConstruction() {
    BaseWidget widget;
    QVERIFY(!widget.styleSheet().isEmpty());
}

void TestBase::testBaseWidgetAppliesStyleSheet() {
    BaseWidget widget;
    // applyTheme() builds a stylesheet referencing background-color and color.
    QString sheet = widget.styleSheet();
    QVERIFY(sheet.contains(QStringLiteral("background-color")));
    QVERIFY(sheet.contains(QStringLiteral("color")));
}

void TestBase::testBaseDialogConstruction() {
    BaseDialog dialog;
    QVERIFY(!dialog.styleSheet().isEmpty());
}

void TestBase::testBaseDialogSetTitle() {
    BaseDialog dialog;
    dialog.setDialogTitle(QStringLiteral("My Title"));
    QLabel* label = dialog.findChild<QLabel*>(QStringLiteral("titleLabel"));
    QVERIFY(label != nullptr);
    QCOMPARE(label->text(), QStringLiteral("My Title"));
}

void TestBase::testBaseDialogAddButton() {
    BaseDialog dialog;
    dialog.addButton(QStringLiteral("OK"));
    QList<QPushButton*> buttons = dialog.findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 1);
    QCOMPARE(buttons.first()->text(), QStringLiteral("OK"));
}

void TestBase::testBaseDialogAddMultipleButtons() {
    BaseDialog dialog;
    dialog.addButton(QStringLiteral("OK"));
    dialog.addButton(QStringLiteral("Cancel"));
    QList<QPushButton*> buttons = dialog.findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 2);
    QCOMPARE(buttons.at(0)->text(), QStringLiteral("OK"));
    QCOMPARE(buttons.at(1)->text(), QStringLiteral("Cancel"));
}

QTEST_MAIN(TestBase)
#include "test_base.moc"
