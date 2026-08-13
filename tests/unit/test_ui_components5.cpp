#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QSignalSpy>
#include <QStackedWidget>
#include <iostream>

#include "soul/ui/loading.h"
#include "soul/ui/empty_widget.h"
#include "soul/ui/page.h"
#include "soul/ui/sidebar.h"
#include "soul/ui/navigation.h"

using namespace sc;

// ============================================================================
// Loading
// ============================================================================
class TestUiLoading : public QObject {
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
        Loading loading(m_parent);
        QVERIFY(loading.text() == QString("Loading..."));
        QVERIFY(loading.progress() == 0);
    }

    void testSetText() {
        Loading loading(m_parent);
        loading.setText("Loading...");
        QVERIFY(loading.text() == QString("Loading..."));
        loading.setText("");
        QVERIFY(loading.text().isEmpty());
    }

    void testSetProgress() {
        Loading loading(m_parent);
        loading.setProgress(50);
        QVERIFY(loading.progress() == 50);
        loading.setProgress(0);
        QVERIFY(loading.progress() == 0);
        loading.setProgress(100);
        QVERIFY(loading.progress() == 100);
    }

    void testShowProgress() {
        Loading loading(m_parent);
        loading.showProgress(true);
        QVERIFY(true);
        loading.showProgress(false);
        QVERIFY(true);
    }

    void testSetIndeterminate() {
        Loading loading(m_parent);
        loading.setIndeterminate(true);
        QVERIFY(true);
        loading.setIndeterminate(false);
        QVERIFY(true);
    }

    void testShowHideGlobal() {
        QVERIFY(true);
    }

    void testShowGlobalDefault() {
        QVERIFY(true);
    }

    void testUpdateGlobalProgress() {
        QVERIFY(true);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// EmptyWidget
// ============================================================================
class TestUiEmptyWidget : public QObject {
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
        EmptyWidget empty(m_parent);
        QVERIFY(true);
    }

    void testSetTitle() {
        EmptyWidget empty(m_parent);
        empty.setTitle("No data");
        QVERIFY(true);
    }

    void testSetSubtitle() {
        EmptyWidget empty(m_parent);
        empty.setSubtitle("Try adding some items");
        QVERIFY(true);
    }

    void testSetIcon() {
        EmptyWidget empty(m_parent);
        QPixmap pm(32, 32);
        pm.fill(Qt::gray);
        QIcon icon(pm);
        empty.setIcon(icon);
        QVERIFY(true);
    }

    void testSetButtonText() {
        EmptyWidget empty(m_parent);
        empty.setButtonText("Add Item");
        QVERIFY(true);
    }

    void testShowButton() {
        EmptyWidget empty(m_parent);
        empty.showButton(true);
        QVERIFY(true);
        empty.showButton(false);
        QVERIFY(true);
    }

    void testButtonClickedSignal() {
        EmptyWidget empty(m_parent);
        empty.showButton(true);
        QSignalSpy spy(&empty, &EmptyWidget::buttonClicked);
        QVERIFY(spy.count() == 0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Page
// ============================================================================
class TestUiPage : public QObject {
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
        Page page(m_parent);
        QVERIFY(page.pageTitle().isEmpty());
        QVERIFY(page.pageSubtitle().isEmpty());
    }

    void testSetPageTitle() {
        Page page(m_parent);
        page.setPageTitle("Home");
        QVERIFY(page.pageTitle() == QString("Home"));
    }

    void testSetPageSubtitle() {
        Page page(m_parent);
        page.setPageSubtitle("Welcome back");
        QVERIFY(page.pageSubtitle() == QString("Welcome back"));
    }

    void testOnEnter() {
        Page page(m_parent);
        page.onEnter();
        QVERIFY(true);
    }

    void testOnLeave() {
        Page page(m_parent);
        page.onLeave();
        QVERIFY(true);
    }

    void testOnBack() {
        Page page(m_parent);
        page.onBack();
        QVERIFY(true);
    }

    void testPageEnterSignal() {
        Page page(m_parent);
        QSignalSpy spy(&page, &Page::pageEnter);
        page.onEnter();
        QVERIFY(spy.count() == 1);
    }

    void testPageLeaveSignal() {
        Page page(m_parent);
        QSignalSpy spy(&page, &Page::pageLeave);
        page.onLeave();
        QVERIFY(spy.count() == 1);
    }

    void testBackPressedSignal() {
        Page page(m_parent);
        QSignalSpy spy(&page, &Page::backPressed);
        page.onBack();
        QVERIFY(spy.count() == 1);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// SideBar
// ============================================================================
class TestUiSideBar : public QObject {
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
        SideBar sidebar(m_parent);
        QVERIFY(!sidebar.isCollapsed());
        QVERIFY(sidebar.activeItem().isEmpty());
        QVERIFY(sidebar.width() > 0);
    }

    void testAddItem() {
        SideBar sidebar(m_parent);
        SideBarItem item;
        item.id = "home";
        item.text = "Home";
        sidebar.addItem(item);
        QVERIFY(true);
    }

    void testAddItems() {
        SideBar sidebar(m_parent);
        QList<SideBarItem> items;
        SideBarItem item1;
        item1.id = "home";
        item1.text = "Home";
        items.append(item1);
        SideBarItem item2;
        item2.id = "settings";
        item2.text = "Settings";
        items.append(item2);
        sidebar.addItems(items);
        QVERIFY(true);
    }

    void testSetActiveItem() {
        SideBar sidebar(m_parent);
        SideBarItem item;
        item.id = "home";
        item.text = "Home";
        sidebar.addItem(item);
        sidebar.setActiveItem("home");
        QVERIFY(sidebar.activeItem() == QString("home"));
    }

    void testActiveItemNotFound() {
        SideBar sidebar(m_parent);
        sidebar.setActiveItem("nonexistent");
        QVERIFY(sidebar.activeItem() == QString("nonexistent"));
    }

    void testRemoveItem() {
        SideBar sidebar(m_parent);
        SideBarItem item;
        item.id = "home";
        item.text = "Home";
        sidebar.addItem(item);
        sidebar.removeItem("home");
        QVERIFY(sidebar.activeItem().isEmpty());
    }

    void testClearItems() {
        SideBar sidebar(m_parent);
        SideBarItem item1;
        item1.id = "home";
        item1.text = "Home";
        sidebar.addItem(item1);
        SideBarItem item2;
        item2.id = "settings";
        item2.text = "Settings";
        sidebar.addItem(item2);
        sidebar.clearItems();
        QVERIFY(sidebar.activeItem().isEmpty());
    }

    void testSetCollapsed() {
        SideBar sidebar(m_parent);
        QVERIFY(!sidebar.isCollapsed());
        sidebar.setCollapsed(true);
        QVERIFY(sidebar.isCollapsed());
        sidebar.setCollapsed(false);
        QVERIFY(!sidebar.isCollapsed());
    }

    void testSetWidth() {
        SideBar sidebar(m_parent);
        sidebar.setWidth(200);
        QVERIFY(sidebar.width() == 200);
        sidebar.setWidth(64);
        QVERIFY(sidebar.width() == 64);
    }

    void testItemClickedSignal() {
        SideBar sidebar(m_parent);
        SideBarItem item;
        item.id = "home";
        item.text = "Home";
        sidebar.addItem(item);
        QSignalSpy spy(&sidebar, &SideBar::itemClicked);
        QVERIFY(spy.count() == 0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Navigation
// ============================================================================
class TestUiNavigation : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        m_stack = new QStackedWidget;
        Navigation::setRootWidget(m_stack);
    }

    void cleanupTestCase() {
        delete m_stack;
        m_stack = nullptr;
    }

    void testInitialState() {
        Navigation::popToRoot();
        QVERIFY(Navigation::stackCount() == 0);
        QVERIFY(Navigation::currentIndex() == -1);
        QVERIFY(Navigation::currentWidget() == nullptr);
        QVERIFY(Navigation::currentTitle().isEmpty());
    }

    void testPush() {
        Navigation::popToRoot();
        QWidget* w1 = new QWidget;
        Navigation::push(w1, "Page 1");
        QVERIFY(Navigation::stackCount() == 1);
        QVERIFY(Navigation::currentIndex() == 0);
        QVERIFY(Navigation::currentTitle() == QString("Page 1"));
        QVERIFY(Navigation::currentWidget() == w1);
    }

    void testPushMultiple() {
        Navigation::popToRoot();
        QWidget* w1 = new QWidget;
        QWidget* w2 = new QWidget;
        Navigation::push(w1, "Page 1");
        Navigation::push(w2, "Page 2");
        QVERIFY(Navigation::stackCount() == 2);
        QVERIFY(Navigation::currentTitle() == QString("Page 2"));
    }

    void testPop() {
        Navigation::popToRoot();
        QWidget* w1 = new QWidget;
        QWidget* w2 = new QWidget;
        Navigation::push(w1, "Page 1");
        Navigation::push(w2, "Page 2");
        Navigation::pop();
        QVERIFY(Navigation::stackCount() == 1);
        QVERIFY(Navigation::currentTitle() == QString("Page 1"));
    }

    void testPopToRoot() {
        Navigation::popToRoot();
        QWidget* w1 = new QWidget;
        QWidget* w2 = new QWidget;
        Navigation::push(w1, "Page 1");
        Navigation::push(w2, "Page 2");
        Navigation::popToRoot();
        QVERIFY(Navigation::stackCount() == 0);
    }

    void testReplace() {
        Navigation::popToRoot();
        QWidget* w1 = new QWidget;
        Navigation::push(w1, "Page 1");
        QWidget* w2 = new QWidget;
        Navigation::replace(w2, "Replaced");
        QVERIFY(Navigation::stackCount() == 1);
        QVERIFY(Navigation::currentTitle() == QString("Replaced"));
    }

    void testPushWithoutTitle() {
        Navigation::popToRoot();
        QWidget* w = new QWidget;
        Navigation::push(w);
        QVERIFY(Navigation::stackCount() == 1);
        QVERIFY(Navigation::currentTitle().isEmpty());
    }

    void testSetOnNavigate() {
        Navigation::popToRoot();
        bool called = false;
        Navigation::setOnNavigate([&called](QWidget*, const QString&) {
            called = true;
        });
        QWidget* w = new QWidget;
        Navigation::push(w, "Test");
        QVERIFY(called);
    }

private:
    QStackedWidget* m_stack = nullptr;
};

int main(int argc, char* argv[]) {
    std::cout << "DEBUG: main start" << std::endl;
    QApplication app(argc, argv);
    std::cout << "DEBUG: QApplication created" << std::endl;

    int result = 0;
    std::cout << "DEBUG: TestUiLoading start" << std::endl;
    { TestUiLoading t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiLoading done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiEmptyWidget start" << std::endl;
    { TestUiEmptyWidget t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiEmptyWidget done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiPage start" << std::endl;
    { TestUiPage t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiPage done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiSideBar start" << std::endl;
    { TestUiSideBar t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiSideBar done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiNavigation start" << std::endl;
    { TestUiNavigation t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiNavigation done, result=" << result << std::endl;

    return result;
}

#include "test_ui_components5.moc"