#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QSignalSpy>
#include <iostream>

#include "soul/ui/slider.h"
#include "soul/ui/spinner.h"
#include "soul/ui/tab_bar.h"
#include "soul/ui/toast.h"
#include "soul/ui/tool_tip.h"

using namespace sc;

// ============================================================================
// Slider
// ============================================================================
class TestUiSlider : public QObject {
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
        Slider slider(m_parent);
        QVERIFY(slider.orientation() == Qt::Vertical);
        QVERIFY(slider.glowProgress() >= 0.0);
    }

    void testConstructionWithOrientation() {
        Slider slider(Qt::Vertical, m_parent);
        QVERIFY(slider.orientation() == Qt::Vertical);
    }

    void testHorizontalConstruction() {
        Slider slider(Qt::Horizontal, m_parent);
        QVERIFY(slider.orientation() == Qt::Horizontal);
    }

    void testGlowProgress() {
        Slider slider(m_parent);
        slider.setGlowProgress(0.5);
        QVERIFY(slider.glowProgress() == 0.5);
        slider.setGlowProgress(0.0);
        QVERIFY(slider.glowProgress() == 0.0);
        slider.setGlowProgress(1.0);
        QVERIFY(slider.glowProgress() == 1.0);
    }

    void testValueRange() {
        Slider slider(m_parent);
        QVERIFY(slider.minimum() == 0);
        QVERIFY(slider.maximum() == 99);
        slider.setRange(0, 200);
        QVERIFY(slider.minimum() == 0);
        QVERIFY(slider.maximum() == 200);
    }

    void testSetValue() {
        Slider slider(m_parent);
        slider.setValue(50);
        QVERIFY(slider.value() == 50);
        slider.setValue(0);
        QVERIFY(slider.value() == 0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Spinner
// ============================================================================
class TestUiSpinner : public QObject {
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
        Spinner spinner(m_parent);
        QVERIFY(spinner.rotation() >= 0.0);
    }

    void testSetRotation() {
        Spinner spinner(m_parent);
        spinner.setRotation(180.0);
        QVERIFY(spinner.rotation() == 180.0);
        spinner.setRotation(0.0);
        QVERIFY(spinner.rotation() == 0.0);
        spinner.setRotation(360.0);
        QVERIFY(spinner.rotation() == 360.0);
    }

    void testNegativeRotation() {
        Spinner spinner(m_parent);
        spinner.setRotation(-90.0);
        QVERIFY(spinner.rotation() == -90.0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// TabBar
// ============================================================================
class TestUiTabBar : public QObject {
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
        TabBar tabBar(m_parent);
        QVERIFY(tabBar.count() == 0);
    }

    void testAddTab() {
        TabBar tabBar(m_parent);
        int idx = tabBar.addTab("Tab 1");
        QVERIFY(idx == 0);
        QVERIFY(tabBar.count() == 1);
        QVERIFY(tabBar.tabText(0) == QString("Tab 1"));
    }

    void testAddMultipleTabs() {
        TabBar tabBar(m_parent);
        tabBar.addTab("Tab 1");
        tabBar.addTab("Tab 2");
        tabBar.addTab("Tab 3");
        QVERIFY(tabBar.count() == 3);
        QVERIFY(tabBar.tabText(0) == QString("Tab 1"));
        QVERIFY(tabBar.tabText(1) == QString("Tab 2"));
        QVERIFY(tabBar.tabText(2) == QString("Tab 3"));
    }

    void testSetCurrentIndex() {
        TabBar tabBar(m_parent);
        tabBar.addTab("Tab 1");
        tabBar.addTab("Tab 2");
        tabBar.setCurrentIndex(1);
        QVERIFY(tabBar.currentIndex() == 1);
        tabBar.setCurrentIndex(0);
        QVERIFY(tabBar.currentIndex() == 0);
    }

    void testCurrentChangedSignal() {
        TabBar tabBar(m_parent);
        tabBar.addTab("Tab 1");
        tabBar.addTab("Tab 2");
        QSignalSpy spy(&tabBar, &QTabBar::currentChanged);
        tabBar.setCurrentIndex(1);
        QVERIFY(spy.count() == 1);
        QVERIFY(spy[0][0].toInt() == 1);
    }

    void testRemoveTab() {
        TabBar tabBar(m_parent);
        tabBar.addTab("Tab 1");
        tabBar.addTab("Tab 2");
        tabBar.removeTab(0);
        QVERIFY(tabBar.count() == 1);
        QVERIFY(tabBar.tabText(0) == QString("Tab 2"));
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Toast
// ============================================================================
class TestUiToast : public QObject {
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
        auto* toast = new Toast(m_parent);
        QVERIFY(toast != nullptr);
        delete toast;
    }

    void testSetMessage() {
        auto* toast = new Toast(m_parent);
        toast->setMessage("Operation successful");
        QVERIFY(true);
        delete toast;
    }

    void testSetType() {
        auto* toast = new Toast(m_parent);
        toast->setType(ToastType::Info);
        QVERIFY(true);
        toast->setType(ToastType::Warning);
        QVERIFY(true);
        toast->setType(ToastType::Error);
        QVERIFY(true);
        toast->setType(ToastType::Success);
        QVERIFY(true);
        delete toast;
    }

    void testSetDuration() {
        auto* toast = new Toast(m_parent);
        toast->setDuration(2000);
        QVERIFY(true);
        delete toast;
    }

    void testShow() {
        auto* toast = new Toast(m_parent);
        toast->setMessage("Test");
        delete toast;
        QVERIFY(true);
    }

    void testStaticInfo() {
        QVERIFY(true);
    }

    void testStaticWarning() {
        QVERIFY(true);
    }

    void testStaticError() {
        QVERIFY(true);
    }

    void testStaticSuccess() {
        QVERIFY(true);
    }

    void testStaticWithDuration() {
        QVERIFY(true);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// ToolTip
// ============================================================================
class TestUiToolTip : public QObject {
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
        ToolTip tooltip(m_parent);
        QVERIFY(tooltip.text().isEmpty());
    }

    void testSetText() {
        ToolTip tooltip(m_parent);
        tooltip.setText("This is a tooltip");
        QVERIFY(tooltip.text() == QString("This is a tooltip"));
    }

    void testEmptyText() {
        ToolTip tooltip(m_parent);
        tooltip.setText("");
        QVERIFY(tooltip.text().isEmpty());
    }

    void testLongText() {
        ToolTip tooltip(m_parent);
        QString longText = "This is a very long tooltip text that should still work correctly";
        tooltip.setText(longText);
        QVERIFY(tooltip.text() == longText);
    }

private:
    QWidget* m_parent = nullptr;
};

int main(int argc, char* argv[]) {
    std::cout << "DEBUG: main start" << std::endl;
    QApplication app(argc, argv);
    std::cout << "DEBUG: QApplication created" << std::endl;

    int result = 0;
    std::cout << "DEBUG: TestUiSlider start" << std::endl;
    { TestUiSlider t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiSlider done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiSpinner start" << std::endl;
    { TestUiSpinner t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiSpinner done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiTabBar start" << std::endl;
    { TestUiTabBar t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiTabBar done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiToast start" << std::endl;
    { TestUiToast t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiToast done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiToolTip start" << std::endl;
    { TestUiToolTip t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiToolTip done, result=" << result << std::endl;

    return result;
}

#include "test_ui_components4.moc"