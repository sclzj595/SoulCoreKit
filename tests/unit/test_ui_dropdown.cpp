#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QSignalSpy>
#include <iostream>

#include "soul/ui/dropdown.h"
#include "soul/ui/tab_bar.h"
#include "soul/ui/tool_tip.h"
#include "soul/ui/scroll_bar.h"

using namespace sc;

// ============================================================================
// Dropdown
// ============================================================================
class TestUiDropdown : public QObject {
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
        Dropdown dd(m_parent);
        QVERIFY(dd.count() == 0);
    }

    void testAddItems() {
        Dropdown dd(m_parent);
        dd.addItem("Option 1");
        dd.addItem("Option 2");
        dd.addItem("Option 3");
        QVERIFY(dd.count() == 3);
    }

    void testCurrentIndex() {
        Dropdown dd(m_parent);
        dd.addItem("A");
        dd.addItem("B");
        dd.addItem("C");
        dd.setCurrentIndex(1);
        QVERIFY(dd.currentIndex() == 1);
    }

    void testCurrentIndexBounds() {
        Dropdown dd(m_parent);
        dd.addItem("A");
        dd.addItem("B");
        dd.setCurrentIndex(0);
        QVERIFY(dd.currentIndex() == 0);
    }

    void testEmptyDropdown() {
        Dropdown dd(m_parent);
        QVERIFY(dd.count() == 0);
        QVERIFY(dd.currentIndex() == -1);
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
        TabBar tb(m_parent);
        QVERIFY(tb.count() == 0);
    }

    void testAddTab() {
        TabBar tb(m_parent);
        tb.addTab("Tab 1");
        tb.addTab("Tab 2");
        tb.addTab("Tab 3");
        QVERIFY(tb.count() == 3);
        QVERIFY(tb.tabText(0) == QString("Tab 1"));
        QVERIFY(tb.tabText(1) == QString("Tab 2"));
        QVERIFY(tb.tabText(2) == QString("Tab 3"));
    }

    void testCurrentIndex() {
        TabBar tb(m_parent);
        tb.addTab("Tab 1");
        tb.addTab("Tab 2");
        tb.setCurrentIndex(1);
        QVERIFY(tb.currentIndex() == 1);
        tb.setCurrentIndex(0);
        QVERIFY(tb.currentIndex() == 0);
    }

    void testEmptyTabText() {
        TabBar tb(m_parent);
        tb.addTab("");
        QVERIFY(tb.tabText(0) == QString(""));
    }

    void testEmptyTabBar() {
        TabBar tb(m_parent);
        QVERIFY(tb.count() == 0);
        QVERIFY(tb.currentIndex() == -1);
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
        ToolTip tt(m_parent);
        QVERIFY(tt.text().isEmpty());
    }

    void testSetText() {
        ToolTip tt(m_parent);
        tt.setText("Tooltip text");
        QVERIFY(tt.text() == QString("Tooltip text"));
    }

    void testEmptyText() {
        ToolTip tt(m_parent);
        tt.setText("");
        QVERIFY(tt.text().isEmpty());
    }

    void testOverrideText() {
        ToolTip tt(m_parent);
        tt.setText("First");
        tt.setText("Second");
        QVERIFY(tt.text() == QString("Second"));
    }

    void testSizeCheck() {
        ToolTip tt(m_parent);
        tt.setText("Hello");
        QVERIFY(tt.width() > 0);
        QVERIFY(tt.height() > 0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// ScrollBar
// ============================================================================
class TestUiScrollBar : public QObject {
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
        ScrollBar sb(m_parent);
        QVERIFY(sb.orientation() == Qt::Vertical);
    }

    void testOrientationConstruction() {
        ScrollBar sb(Qt::Vertical, m_parent);
        QVERIFY(sb.orientation() == Qt::Vertical);
    }

    void testHorizontalOrientation() {
        ScrollBar sb(Qt::Horizontal, m_parent);
        QVERIFY(sb.orientation() == Qt::Horizontal);
    }

    void testValueRange() {
        ScrollBar sb(m_parent);
        sb.setRange(0, 100);
        sb.setValue(50);
        QVERIFY(sb.value() == 50);
    }

    void testRangeEdgeCases() {
        ScrollBar sb(m_parent);
        sb.setRange(0, 0);
        sb.setValue(0);
        QVERIFY(sb.value() == 0);
    }

    void testSetValueBoundaries() {
        ScrollBar sb(m_parent);
        sb.setRange(0, 100);
        sb.setValue(0);
        QVERIFY(sb.value() == 0);
        sb.setValue(100);
        QVERIFY(sb.value() == 100);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// main
// ============================================================================
int main(int argc, char* argv[]) {
    std::cout << "DEBUG: main start" << std::endl;
    QApplication app(argc, argv);
    std::cout << "DEBUG: QApplication created" << std::endl;
    int result = 0;

    std::cout << "DEBUG: TestUiDropdown start" << std::endl;
    { TestUiDropdown t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiDropdown done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiTabBar start" << std::endl;
    { TestUiTabBar t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiTabBar done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiToolTip start" << std::endl;
    { TestUiToolTip t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiToolTip done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiScrollBar start" << std::endl;
    { TestUiScrollBar t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiScrollBar done, result=" << result << std::endl;

    return result;
}

#include "test_ui_dropdown.moc"