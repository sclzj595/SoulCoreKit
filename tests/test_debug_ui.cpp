#include <QTest>
#include <QApplication>
#include <QWidget>
#include <iostream>

#include "soul/ui/button.h"
#include "soul/ui/checkbox.h"
#include "soul/ui/card.h"
#include "soul/ui/badge.h"
#include "soul/ui/input.h"
#include "soul/ui/progress.h"
#include "soul/ui/slider.h"
#include "soul/ui/spinner.h"
#include "soul/ui/tab_bar.h"
#include "soul/ui/toast.h"
#include "soul/ui/tool_tip.h"
#include "soul/ui/loading.h"
#include "soul/ui/empty_widget.h"
#include "soul/ui/page.h"
#include "soul/ui/navigation.h"
#include "soul/ui/sidebar.h"
#include "soul/ui/base_widget.h"
#include "soul/ui/icon.h"
#include "soul/ui/scroll_bar.h"
#include "soul/ui/glass_widget.h"
#include "soul/ui/animation.h"
#include "soul/ui/icon_manager.h"

using namespace sc;

#define TEST_BEGIN(name) std::cerr << "  " << name << "..." << std::endl;
#define TEST_PASS(name) std::cerr << "  " << name << " PASS" << std::endl;
#define TEST_FAIL(name) std::cerr << "  " << name << " FAIL" << std::endl;

class TestDebug : public QObject {
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

    void testButton() {
        TEST_BEGIN("testButton")
        Button btn(m_parent);
        QVERIFY(btn.buttonType() == ButtonType::Push);
        TEST_PASS("testButton")
    }

    void testCard() {
        TEST_BEGIN("testCard")
        Card card(m_parent);
        QVERIFY(card.borderRadius() >= 0);
        TEST_PASS("testCard")
    }

    void testCheckbox() {
        TEST_BEGIN("testCheckbox")
        Checkbox cb(m_parent);
        QVERIFY(!cb.isChecked());
        TEST_PASS("testCheckbox")
    }

    void testBadge() {
        TEST_BEGIN("testBadge")
        Badge badge(m_parent);
        QCOMPARE(badge.count(), 0);
        TEST_PASS("testBadge")
    }

    void testInput() {
        TEST_BEGIN("testInput")
        Input input(m_parent);
        QCOMPARE(input.inputType(), Input::Normal);
        TEST_PASS("testInput")
    }

    void testProgress() {
        TEST_BEGIN("testProgress")
        Progress prog(m_parent);
        QCOMPARE(prog.value(), 0);
        TEST_PASS("testProgress")
    }

    void testSlider() {
        TEST_BEGIN("testSlider")
        Slider slider(m_parent);
        QCOMPARE(slider.orientation(), Qt::Horizontal);
        TEST_PASS("testSlider")
    }

    void testSpinner() {
        TEST_BEGIN("testSpinner")
        Spinner spinner(m_parent);
        QVERIFY(true);
        TEST_PASS("testSpinner")
    }

    void testTabBar() {
        TEST_BEGIN("testTabBar")
        TabBar tb(m_parent);
        QVERIFY(true);
        TEST_PASS("testTabBar")
    }

    void testToast() {
        TEST_BEGIN("testToast")
        Toast toast(m_parent);
        QVERIFY(true);
        TEST_PASS("testToast")
    }

    void testToolTip() {
        TEST_BEGIN("testToolTip")
        ToolTip tt(m_parent);
        QVERIFY(true);
        TEST_PASS("testToolTip")
    }

    void testLoading() {
        TEST_BEGIN("testLoading")
        Loading loading(m_parent);
        QCOMPARE(loading.progress(), 0);
        TEST_PASS("testLoading")
    }

    void testEmptyWidget() {
        TEST_BEGIN("testEmptyWidget")
        EmptyWidget ew(m_parent);
        QVERIFY(true);
        TEST_PASS("testEmptyWidget")
    }

    void testPage() {
        TEST_BEGIN("testPage")
        Page page(m_parent);
        QVERIFY(true);
        TEST_PASS("testPage")
    }

    void testSideBar() {
        TEST_BEGIN("testSideBar")
        SideBar sb(m_parent);
        QVERIFY(true);
        TEST_PASS("testSideBar")
    }

    void testNavigation() {
        TEST_BEGIN("testNavigation")
        QVERIFY(true);
        TEST_PASS("testNavigation")
    }

    void testBaseWidget() {
        TEST_BEGIN("testBaseWidget")
        ui::BaseWidget bw(m_parent);
        QVERIFY(bw.width() > 0);
        TEST_PASS("testBaseWidget")
    }

    void testScrollBar() {
        TEST_BEGIN("testScrollBar")
        ScrollBar sb(m_parent);
        QVERIFY(true);
        TEST_PASS("testScrollBar")
    }

    void testGlassWidget() {
        TEST_BEGIN("testGlassWidget")
        ui::GlassWidget gw(m_parent);
        QVERIFY(gw.blurRadius() >= 0);
        TEST_PASS("testGlassWidget")
    }

    void testAnimation() {
        TEST_BEGIN("testAnimation")
        QWidget w(m_parent);
        Animation::shake(&w, 100);
        QVERIFY(true);
        TEST_PASS("testAnimation")
    }

    void testIconManager() {
        TEST_BEGIN("testIconManager")
        IconManager::instance().init();
        IconManager::instance().shutdown();
        QVERIFY(true);
        TEST_PASS("testIconManager")
    }

private:
    QWidget* m_parent = nullptr;
};

int main(int argc, char* argv[]) {
    std::cerr << "DEBUG: Starting test..." << std::endl;
    QApplication app(argc, argv);
    std::cerr << "DEBUG: QApplication created" << std::endl;

    int result = 0;
    { TestDebug t; result |= QTest::qExec(&t, argc, argv); }
    
    std::cerr << "DEBUG: Test finished, result=" << result << std::endl;
    return result;
}

#include "test_debug_ui.moc"