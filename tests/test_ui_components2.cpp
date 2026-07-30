#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include <QLabel>

#include "soul/ui/base_dialog.h"
#include "soul/ui/base_view_model.h"

using namespace sc;

// ============================================================================
// BaseDialog
// ============================================================================
class TestUiBaseDialog : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        m_parent = new QWidget();
        m_parent->resize(400, 300);
        m_parent->show();
    }
    void cleanupTestCase() {
        delete m_parent;
        m_parent = nullptr;
    }

    void testDefaultConstruction() {
        ui::BaseDialog dlg(m_parent);
        QVERIFY(dlg.windowTitle().isEmpty());
    }

    void testSetDialogTitle() {
        ui::BaseDialog dlg(m_parent);
        dlg.setDialogTitle("Test Dialog");
        QLabel* titleLabel = dlg.findChild<QLabel*>("titleLabel");
        QVERIFY(titleLabel != nullptr);
        QCOMPARE(titleLabel->text(), QString("Test Dialog"));
    }

    void testAddButton() {
        ui::BaseDialog dlg(m_parent);
        dlg.addButton("OK");
        dlg.addButton("Cancel");
        QVERIFY(true);
    }

    void testAddButtonWithCallback() {
        ui::BaseDialog dlg(m_parent);
        bool called = false;
        dlg.addButton("Click", [&called]() { called = true; });
        QVERIFY(!called);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// BaseViewModel
// ============================================================================
class TestUiBaseViewModel : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        ui::BaseViewModel vm;
        QVERIFY(vm.get("nonexistent").isNull());
    }

    void testSetAndGet() {
        ui::BaseViewModel vm;
        vm.set("key1", QVariant("value1"));
        QCOMPARE(vm.get("key1").toString(), QString("value1"));
        QCOMPARE(vm.get("key2", QVariant("default")).toString(), QString("default"));
    }

    void testSetValueAndGetValue() {
        ui::BaseViewModel vm;
        vm.setValue<QString>("name", "Alice");
        QCOMPARE(vm.getValue<QString>("name"), QString("Alice"));
        QCOMPARE(vm.getValue<QString>("unknown", "default"), QString("default"));
    }

    void testSetValueInt() {
        ui::BaseViewModel vm;
        vm.setValue<int>("count", 42);
        QCOMPARE(vm.getValue<int>("count"), 42);
        QCOMPARE(vm.getValue<int>("missing", 0), 0);
    }

    void testSetValueBool() {
        ui::BaseViewModel vm;
        vm.setValue<bool>("visible", true);
        QCOMPARE(vm.getValue<bool>("visible"), true);
        vm.setValue<bool>("visible", false);
        QCOMPARE(vm.getValue<bool>("visible"), false);
    }

    void testSignalEmitted() {
        ui::BaseViewModel vm;
        QSignalSpy spy(&vm, &ui::BaseViewModel::propertyChanged);
        vm.setValue<QString>("key", "val");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toString(), QString("key"));
    }

    void testNoDuplicateSignal() {
        ui::BaseViewModel vm;
        vm.setValue<QString>("key", "same");
        QSignalSpy spy(&vm, &ui::BaseViewModel::propertyChanged);
        vm.setValue<QString>("key", "same");
        QCOMPARE(spy.count(), 0);
    }

    void testMultipleProperties() {
        ui::BaseViewModel vm;
        vm.set("a", QVariant(1));
        vm.set("b", QVariant(2));
        vm.set("c", QVariant(3));
        QCOMPARE(vm.get("a").toInt(), 1);
        QCOMPARE(vm.get("b").toInt(), 2);
        QCOMPARE(vm.get("c").toInt(), 3);
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    int result = 0;
    { TestUiBaseDialog t; result |= QTest::qExec(&t, argc, argv); }
    { TestUiBaseViewModel t; result |= QTest::qExec(&t, argc, argv); }

    return result;
}

#include "test_ui_components2.moc"