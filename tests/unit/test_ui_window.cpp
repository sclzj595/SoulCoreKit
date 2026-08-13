#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QColor>
#include <iostream>

#include "soul/ui/window.h"
#include "soul/ui/icon.h"
#include "soul/ui/navigation.h"
#include "soul/ui/page.h"
#include "soul/ui/base_view.h"
#include "soul/ui/sidebar.h"
#include "soul/ui/empty_widget.h"

using namespace sc;

// ============================================================================
// Window
// ============================================================================
class TestUiWindow : public QObject {
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
        Window win(m_parent);
        QVERIFY(win.title().isEmpty());
        QVERIFY(win.isFrameless());
        QVERIFY(win.hasGlassEffect());
        QVERIFY(win.hasGlowBorder());
    }

    void testTitle() {
        Window win(m_parent);
        win.setTitle("My App");
        QVERIFY(win.title() == QString("My App"));
        win.setTitle("");
        QVERIFY(win.title().isEmpty());
    }

    void testFrameless() {
        Window win(m_parent);
        win.setFrameless(true);
        QVERIFY(win.isFrameless());
        win.setFrameless(false);
        QVERIFY(!win.isFrameless());
    }

    void testGlassEffect() {
        Window win(m_parent);
        win.setGlassEffect(true);
        QVERIFY(win.hasGlassEffect());
        win.setGlassEffect(false);
        QVERIFY(!win.hasGlassEffect());
    }

    void testGlowBorder() {
        Window win(m_parent);
        win.setGlowBorder(true);
        QVERIFY(win.hasGlowBorder());
        win.setGlowBorder(false);
        QVERIFY(!win.hasGlowBorder());
    }

    void testBlurRadius() {
        Window win(m_parent);
        win.setBlurRadius(20);
        QVERIFY(win.blurRadius() == 20);
        win.setBlurRadius(0);
        QVERIFY(win.blurRadius() == 0);
    }

    void testTintColor() {
        Window win(m_parent);
        win.setTintColor(QColor(255, 255, 255, 128));
        QVERIFY(win.tintColor() == QColor(255, 255, 255, 128));
        win.setTintColor(QColor(0, 0, 0, 64));
        QVERIFY(win.tintColor() == QColor(0, 0, 0, 64));
    }

    void testWindowSignals() {
        Window win(m_parent);
        QSignalSpy closedSpy(&win, &Window::windowClosed);
        QSignalSpy minSpy(&win, &Window::windowMinimized);
        QSignalSpy maxSpy(&win, &Window::windowMaximized);
        QVERIFY(closedSpy.count() == 0);
        QVERIFY(minSpy.count() == 0);
        QVERIFY(maxSpy.count() == 0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Icon
// ============================================================================
class TestUiIcon : public QObject {
    Q_OBJECT
private slots:
    void testFromColor() {
        QIcon icon = Icon::fromColor(QColor(255, 0, 0));
        QVERIFY(!icon.isNull());
    }

    void testFromColorDefaultSize() {
        QIcon icon = Icon::fromColor(QColor(0, 255, 0));
        QVERIFY(!icon.isNull());
    }

    void testFromColorDifferentSizes() {
        QVERIFY(!Icon::fromColor(QColor(0, 0, 255), 16).isNull());
        QVERIFY(!Icon::fromColor(QColor(0, 0, 255), 32).isNull());
        QVERIFY(!Icon::fromColor(QColor(0, 0, 255), 64).isNull());
    }

    void testAppIcon() {
        QVERIFY(true);
    }

    void testSetAppIcon() {
        QIcon customIcon = Icon::fromColor(QColor(0, 255, 0));
        Icon::setAppIcon(customIcon);
        QVERIFY(!Icon::appIcon().isNull());
    }

    void testFromResource() {
        QIcon icon = Icon::fromResource(":/icons/empty.png");
        Q_UNUSED(icon);
        QVERIFY(true);
    }

    void testFromFont() {
        QIcon icon = Icon::fromFont("MaterialIcons", "\uE87C");
        Q_UNUSED(icon);
        QVERIFY(true);
    }
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

    void testSetRootWidget() {
        QVERIFY(true);
    }

    void testPushPop() {
        Navigation::popToRoot();
        Navigation::push(new QWidget(), "Page 1");
        QVERIFY(Navigation::stackCount() == 1);
        QVERIFY(Navigation::currentIndex() == 0);
        QVERIFY(Navigation::currentTitle() == QString("Page 1"));
        Navigation::push(new QWidget(), "Page 2");
        QVERIFY(Navigation::stackCount() == 2);
        Navigation::pop();
        QVERIFY(Navigation::stackCount() == 1);
        QVERIFY(Navigation::currentTitle() == QString("Page 1"));
    }

    void testPopToRoot() {
        Navigation::popToRoot();
        Navigation::push(new QWidget(), "Page 1");
        Navigation::push(new QWidget(), "Page 2");
        Navigation::push(new QWidget(), "Page 3");
        QVERIFY(Navigation::stackCount() == 3);
        Navigation::popToRoot();
        QVERIFY(Navigation::stackCount() == 0);
    }

    void testStackCount() {
        Navigation::popToRoot();
        QVERIFY(Navigation::stackCount() == 0);
        Navigation::push(new QWidget(), "Page 1");
        QVERIFY(Navigation::stackCount() == 1);
    }

    void testCurrentWidget() {
        Navigation::popToRoot();
        auto* widget = new QWidget();
        Navigation::push(widget, "Test");
        QVERIFY(Navigation::currentWidget() == widget);
    }

    void testReplace() {
        Navigation::popToRoot();
        Navigation::push(new QWidget(), "Page 1");
        Navigation::replace(new QWidget(), "New Page");
        QVERIFY(Navigation::stackCount() == 1);
        QVERIFY(Navigation::currentTitle() == QString("New Page"));
    }

    void testPushWithoutTitle() {
        Navigation::popToRoot();
        Navigation::push(new QWidget());
        QVERIFY(Navigation::stackCount() == 1);
        QVERIFY(Navigation::currentTitle().isEmpty());
        Navigation::pop();
        QVERIFY(Navigation::stackCount() == 1);
        Navigation::popToRoot();
        QVERIFY(Navigation::stackCount() == 0);
    }

    void testOnNavigateCallback() {
        Navigation::popToRoot();
        bool called = false;
        Navigation::setOnNavigate([&](QWidget*, const QString&) { called = true; });
        Navigation::push(new QWidget(), "Page");
        QVERIFY(called);
    }

private:
    QStackedWidget* m_stack = nullptr;
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

    void testPageTitle() {
        Page page(m_parent);
        page.setPageTitle("Home");
        QVERIFY(page.pageTitle() == QString("Home"));
        page.setPageTitle("");
        QVERIFY(page.pageTitle().isEmpty());
    }

    void testPageSubtitle() {
        Page page(m_parent);
        page.setPageSubtitle("Manage your preferences");
        QVERIFY(page.pageSubtitle() == QString("Manage your preferences"));
        page.setPageSubtitle("");
        QVERIFY(page.pageSubtitle().isEmpty());
    }

    void testLifecycle() {
        class TestPageDerived : public Page {
        public:
            bool entered = false, left = false, back = false;
            TestPageDerived(QWidget* p) : Page(p) {}
            void onEnter() override { entered = true; }
            void onLeave() override { left = true; }
            void onBack() override { back = true; }
        };
        TestPageDerived page(m_parent);
        page.onEnter(); QVERIFY(page.entered);
        page.onLeave(); QVERIFY(page.left);
        page.onBack(); QVERIFY(page.back);
    }

    void testPageEnterSignal() {
        Page page(m_parent);
        QSignalSpy spy(&page, &Page::pageEnter);
        QVERIFY(spy.count() == 0);
    }

    void testPageLeaveSignal() {
        Page page(m_parent);
        QSignalSpy spy(&page, &Page::pageLeave);
        QVERIFY(spy.count() == 0);
    }

    void testBackPressedSignal() {
        Page page(m_parent);
        QSignalSpy spy(&page, &Page::backPressed);
        QVERIFY(spy.count() == 0);
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
        EmptyWidget ew(m_parent);
        QVERIFY(true);
    }

    void testSetTitle() {
        EmptyWidget ew(m_parent);
        ew.setTitle("No data");
        QVERIFY(true);
    }

    void testSetSubtitle() {
        EmptyWidget ew(m_parent);
        ew.setSubtitle("Please add some items");
        QVERIFY(true);
    }

    void testSetIcon() {
        EmptyWidget ew(m_parent);
        QPixmap pm(32, 32);
        pm.fill(Qt::blue);
        ew.setIcon(QIcon(pm));
        QVERIFY(true);
    }

    void testButtonText() {
        EmptyWidget ew(m_parent);
        ew.setButtonText("Add Item");
        ew.setButtonText("");
        QVERIFY(true);
    }

    void testShowButton() {
        EmptyWidget ew(m_parent);
        ew.showButton(true);
        ew.showButton(false);
        QVERIFY(true);
    }

    void testButtonClickedSignal() {
        EmptyWidget ew(m_parent);
        QSignalSpy spy(&ew, &EmptyWidget::buttonClicked);
        QVERIFY(spy.count() == 0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// BaseView
// ============================================================================
class TestUiBaseView : public QObject {
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
        BaseView view(m_parent);
        QVERIFY(view.title().isEmpty());
    }

    void testShowHide() {
        BaseView view(m_parent);
        view.show();
        QVERIFY(view.isVisible());
        view.hide();
        QVERIFY(!view.isVisible());
    }

    void testWidget() {
        BaseView view(m_parent);
        QVERIFY(view.widget() == &view);
    }

    void testSetTitle() {
        BaseView view(m_parent);
        view.setTitle("My View");
        QVERIFY(view.title() == QString("My View"));
        view.setTitle("");
        QVERIFY(view.title().isEmpty());
    }

    void testSetSize() {
        BaseView view(m_parent);
        view.setSize(800, 600);
        QVERIFY(view.size() == QSize(800, 600));
        view.setSize(1024, 768);
        QVERIFY(view.size() == QSize(1024, 768));
    }

    void testSetPosition() {
        BaseView view(m_parent);
        view.setPosition(100, 200);
        QVERIFY(view.position() == QPoint(100, 200));
        view.setPosition(0, 0);
        QVERIFY(view.position() == QPoint(0, 0));
    }

    void testClose() {
        BaseView view(m_parent);
        view.show();
        QVERIFY(view.isVisible());
        view.close();
        QVERIFY(!view.isVisible());
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
        SideBar sb(m_parent);
        QVERIFY(sb.activeItem().isEmpty());
        QVERIFY(!sb.isCollapsed());
        QVERIFY(sb.width() > 0);
    }

    void testAddItem() {
        SideBar sb(m_parent);
        SideBarItem item;
        item.id = "home";
        item.text = "Home";
        sb.addItem(item);
        QVERIFY(true);
    }

    void testAddItems() {
        SideBar sb(m_parent);
        QList<SideBarItem> items;
        SideBarItem item1; item1.id = "item1"; item1.text = "Item 1"; items.append(item1);
        SideBarItem item2; item2.id = "item2"; item2.text = "Item 2"; items.append(item2);
        SideBarItem item3; item3.id = "item3"; item3.text = "Item 3"; items.append(item3);
        sb.addItems(items);
        QVERIFY(true);
    }

    void testRemoveItem() {
        SideBar sb(m_parent);
        SideBarItem item; item.id = "temp"; item.text = "Temp";
        sb.addItem(item);
        sb.removeItem("temp");
        sb.removeItem("nonexistent");
        QVERIFY(true);
    }

    void testActiveItem() {
        SideBar sb(m_parent);
        SideBarItem item; item.id = "active"; item.text = "Active";
        sb.addItem(item);
        sb.setActiveItem("active");
        QVERIFY(sb.activeItem() == QString("active"));
        sb.setActiveItem("");
        QVERIFY(sb.activeItem().isEmpty());
    }

    void testCollapsed() {
        SideBar sb(m_parent);
        sb.setCollapsed(true);
        QVERIFY(sb.isCollapsed());
        sb.setCollapsed(false);
        QVERIFY(!sb.isCollapsed());
    }

    void testSetWidth() {
        SideBar sb(m_parent);
        sb.setWidth(200);
        QVERIFY(sb.width() == 200);
        sb.setWidth(300);
        QVERIFY(sb.width() == 300);
    }

    void testClearItems() {
        SideBar sb(m_parent);
        SideBarItem item; item.id = "test"; item.text = "Test";
        sb.addItem(item);
        sb.clearItems();
        QVERIFY(true);
    }

    void testItemClickedSignal() {
        SideBar sb(m_parent);
        QSignalSpy spy(&sb, &SideBar::itemClicked);
        QVERIFY(spy.count() == 0);
    }

    void testSetItemIconColor() {
        SideBar sb(m_parent);
        SideBarItem item; item.id = "colored"; item.text = "Colored";
        sb.addItem(item);
        sb.setItemIconColor("colored", QColor(255, 0, 0));
        sb.setItemIconColor("colored", QColor(0, 255, 0));
        QVERIFY(true);
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

    std::cout << "DEBUG: TestUiWindow start" << std::endl;
    { TestUiWindow t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiWindow done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiIcon start" << std::endl;
    { TestUiIcon t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiIcon done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiNavigation start" << std::endl;
    { TestUiNavigation t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiNavigation done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiPage start" << std::endl;
    { TestUiPage t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiPage done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiEmptyWidget start" << std::endl;
    { TestUiEmptyWidget t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiEmptyWidget done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiBaseView start" << std::endl;
    { TestUiBaseView t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiBaseView done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiSideBar start" << std::endl;
    { TestUiSideBar t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiSideBar done, result=" << result << std::endl;

    return result;
}

#include "test_ui_window.moc"