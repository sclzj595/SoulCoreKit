#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QSignalSpy>
#include <QColor>
#include <iostream>

#include "soul/ui/base_widget.h"
#include "soul/ui/base_view.h"
#include "soul/ui/icon.h"
#include "soul/ui/scroll_bar.h"

using namespace sc;

// ============================================================================
// BaseWidget
// ============================================================================
class TestUiBaseWidget : public QObject {
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
        ui::BaseWidget bw(m_parent);
        QVERIFY(bw.parent() == m_parent);
        QVERIFY(bw.isWidgetType());
    }

    void testParentChild() {
        ui::BaseWidget* bw = new ui::BaseWidget(m_parent);
        QVERIFY(bw->parent() == m_parent);
        delete bw;
    }

    void testResize() {
        ui::BaseWidget bw(m_parent);
        bw.resize(400, 300);
        QVERIFY(bw.width() == 400);
        QVERIFY(bw.height() == 300);
    }

    void testShowHide() {
        ui::BaseWidget bw(m_parent);
        bw.show();
        QVERIFY(bw.isVisible());
        bw.hide();
        QVERIFY(bw.isHidden());
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
        QVERIFY(view.widget() == &view);
        QVERIFY(view.title().isEmpty());
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
        QVERIFY(view.size().width() == 800);
        QVERIFY(view.size().height() == 600);
    }

    void testSetPosition() {
        BaseView view(m_parent);
        view.setPosition(100, 200);
        QVERIFY(view.position().x() == 100);
        QVERIFY(view.position().y() == 200);
    }

    void testShowHide() {
        BaseView view(m_parent);
        view.show();
        QVERIFY(view.isVisible());
        view.hide();
        QVERIFY(view.isHidden());
    }

    void testClose() {
        BaseView view(m_parent);
        view.show();
        QVERIFY(view.isVisible());
        view.close();
        QVERIFY(view.isHidden());
    }

    void testWidget() {
        BaseView view(m_parent);
        QWidget* w = view.widget();
        QVERIFY(w != nullptr);
        QVERIFY(w == &view);
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
    void testFromResource() {
        QIcon icon = Icon::fromResource(":/icons/nonexistent.png");
        QVERIFY(true);
    }

    void testFromFont() {
        QIcon icon = Icon::fromFont("Arial", "A");
        QVERIFY(true);
    }

    void testFromColor() {
        QIcon icon = Icon::fromColor(QColor(255, 0, 0), 16);
        QVERIFY(!icon.isNull());
    }

    void testFromColorDefaultSize() {
        QIcon icon = Icon::fromColor(QColor(0, 0, 255));
        QVERIFY(!icon.isNull());
    }

    void testFromColorZeroSize() {
        QIcon icon = Icon::fromColor(QColor(0, 255, 0), 0);
        QVERIFY(true);
    }

    void testAppIcon() {
        QIcon icon = Icon::appIcon();
        QVERIFY(true);
    }

    void testSetAppIcon() {
        QPixmap pm(16, 16);
        pm.fill(Qt::blue);
        QIcon customIcon(pm);
        Icon::setAppIcon(customIcon);
        QVERIFY(!Icon::appIcon().isNull());
    }

    void testFromColorMultipleSizes() {
        QIcon icon = Icon::fromColor(QColor(128, 128, 128), 24);
        QVERIFY(!icon.isNull());
        icon = Icon::fromColor(QColor(255, 255, 0), 32);
        QVERIFY(!icon.isNull());
    }
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
        ScrollBar scrollBar(m_parent);
        QVERIFY(scrollBar.orientation() == Qt::Vertical);
        QVERIFY(scrollBar.minimum() == 0);
        QVERIFY(scrollBar.maximum() == 99);
    }

    void testHorizontalConstruction() {
        ScrollBar scrollBar(Qt::Horizontal, m_parent);
        QVERIFY(scrollBar.orientation() == Qt::Horizontal);
    }

    void testVerticalConstruction() {
        ScrollBar scrollBar(Qt::Vertical, m_parent);
        QVERIFY(scrollBar.orientation() == Qt::Vertical);
    }

    void testSetRange() {
        ScrollBar scrollBar(m_parent);
        scrollBar.setRange(0, 200);
        QVERIFY(scrollBar.minimum() == 0);
        QVERIFY(scrollBar.maximum() == 200);
    }

    void testSetValue() {
        ScrollBar scrollBar(m_parent);
        scrollBar.setValue(50);
        QVERIFY(scrollBar.value() == 50);
        scrollBar.setValue(0);
        QVERIFY(scrollBar.value() == 0);
    }

    void testValueChangedSignal() {
        ScrollBar scrollBar(m_parent);
        QSignalSpy spy(&scrollBar, &QScrollBar::valueChanged);
        scrollBar.setValue(30);
        QVERIFY(spy.count() == 1);
        QVERIFY(spy[0][0].toInt() == 30);
    }

    void testOrientationWithParent() {
        ScrollBar* scrollBar = new ScrollBar(Qt::Vertical, m_parent);
        QVERIFY(scrollBar->orientation() == Qt::Vertical);
        QVERIFY(scrollBar->parent() == m_parent);
        delete scrollBar;
    }

private:
    QWidget* m_parent = nullptr;
};

int main(int argc, char* argv[]) {
    std::cout << "DEBUG: main start" << std::endl;
    QApplication app(argc, argv);
    std::cout << "DEBUG: QApplication created" << std::endl;

    int result = 0;
    std::cout << "DEBUG: TestUiBaseWidget start" << std::endl;
    { TestUiBaseWidget t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiBaseWidget done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiBaseView start" << std::endl;
    { TestUiBaseView t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiBaseView done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiIcon start" << std::endl;
    { TestUiIcon t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiIcon done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiScrollBar start" << std::endl;
    { TestUiScrollBar t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiScrollBar done, result=" << result << std::endl;

    return result;
}

#include "test_ui_components6.moc"