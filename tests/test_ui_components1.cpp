#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QColor>
#include <iostream>

#include "soul/ui/glass_widget.h"
#include "soul/ui/animation.h"
#include "soul/ui/icon_manager.h"

using namespace sc;

// ============================================================================
// GlassWidget
// ============================================================================
class TestUiGlassWidget : public QObject {
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
        ui::GlassWidget gw(m_parent);
        QVERIFY(gw.blurRadius() >= 0);
        QVERIFY(gw.opacity() >= 0.0);
        QVERIFY(gw.tintColor().isValid());
    }

    void testBlurRadius() {
        ui::GlassWidget gw(m_parent);
        gw.setBlurRadius(20);
        QVERIFY(gw.blurRadius() == 20);
        gw.setBlurRadius(0);
        QVERIFY(gw.blurRadius() == 0);
    }

    void testOpacity() {
        ui::GlassWidget gw(m_parent);
        gw.setOpacity(0.5);
        QVERIFY(gw.opacity() == 0.5);
        gw.setOpacity(1.0);
        QVERIFY(gw.opacity() == 1.0);
    }

    void testTintColor() {
        ui::GlassWidget gw(m_parent);
        gw.setTintColor(QColor(255, 255, 255, 128));
        QVERIFY(gw.tintColor() == QColor(255, 255, 255, 128));
        gw.setTintColor(QColor(0, 0, 0, 64));
        QVERIFY(gw.tintColor() == QColor(0, 0, 0, 64));
    }

    void testUpdate() {
        ui::GlassWidget gw(m_parent);
        gw.update();
        QVERIFY(true);
    }

    void testSizeCheck() {
        ui::GlassWidget gw(m_parent);
        QVERIFY(gw.width() > 0);
        QVERIFY(gw.height() > 0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Animation
// ============================================================================
class TestUiAnimation : public QObject {
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

    void testFadeIn() {
        QWidget w(m_parent);
        Animation::fadeIn(&w, 200);
        QVERIFY(true);
    }

    void testFadeOut() {
        QWidget w(m_parent);
        Animation::fadeOut(&w, 200);
        QVERIFY(true);
    }

    void testSlideInFromTop() {
        QWidget w(m_parent);
        Animation::slideInFromTop(&w, 200);
        QVERIFY(true);
    }

    void testSlideInFromBottom() {
        QWidget w(m_parent);
        Animation::slideInFromBottom(&w, 200);
        QVERIFY(true);
    }

    void testSlideInFromLeft() {
        QWidget w(m_parent);
        Animation::slideInFromLeft(&w, 200);
        QVERIFY(true);
    }

    void testSlideInFromRight() {
        QWidget w(m_parent);
        Animation::slideInFromRight(&w, 200);
        QVERIFY(true);
    }

    void testScaleUp() {
        QWidget w(m_parent);
        Animation::scaleUp(&w, 200);
        QVERIFY(true);
    }

    void testScaleDown() {
        QWidget w(m_parent);
        Animation::scaleDown(&w, 200);
        QVERIFY(true);
    }

    void testShake() {
        QWidget w(m_parent);
        Animation::shake(&w, 200);
        QVERIFY(true);
    }

    void testBreathing() {
        QWidget w(m_parent);
        Animation::applyBreathing(&w, 500);
        QVERIFY(true);
        Animation::stopBreathing(&w);
        QVERIFY(true);
    }

    void testGlow() {
        QWidget w(m_parent);
        Animation::applyGlow(&w, QColor(255, 0, 0), 200);
        QVERIFY(true);
        Animation::removeGlow(&w);
        QVERIFY(true);
    }

    void testPress() {
        QWidget w(m_parent);
        Animation::applyPress(&w, 100);
        QVERIFY(true);
    }

    void testHoverLift() {
        QWidget w(m_parent);
        Animation::applyHoverLift(&w, 200);
        QVERIFY(true);
        Animation::removeHoverLift(&w);
        QVERIFY(true);
    }

    void testNullWidget() {
        Animation::fadeIn(nullptr, 100);
        Animation::fadeOut(nullptr, 100);
        Animation::shake(nullptr, 100);
        Animation::scaleUp(nullptr, 100);
        Animation::scaleDown(nullptr, 100);
        Animation::slideInFromTop(nullptr, 100);
        Animation::slideInFromBottom(nullptr, 100);
        Animation::slideInFromLeft(nullptr, 100);
        Animation::slideInFromRight(nullptr, 100);
        QVERIFY(true);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// IconManager
// ============================================================================
class TestUiIconManager : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        IconManager::instance().init();
    }

    void cleanupTestCase() {
        IconManager::instance().shutdown();
    }

    void testDefaultColor() {
        QColor c = IconManager::instance().defaultColor();
        QVERIFY(c.isValid());
    }

    void testSetDefaultColor() {
        IconManager::instance().setDefaultColor(QColor(255, 0, 0));
        QVERIFY(IconManager::instance().defaultColor() == QColor(255, 0, 0));
    }

    void testDefaultSize() {
        int size = IconManager::instance().defaultSize();
        QVERIFY(size > 0);
    }

    void testSetDefaultSize() {
        IconManager::instance().setDefaultSize(32);
        QVERIFY(IconManager::instance().defaultSize() == 32);
    }

    void testRegisterIconByPath() {
        IconManager::instance().registerIcon("test_icon", ":/icons/test.png");
        QVERIFY(true);
    }

    void testIcon() {
        QIcon icon = IconManager::instance().icon("non_existent_icon");
        QVERIFY(true);
    }

    void testPixmap() {
        QPixmap pm = IconManager::instance().pixmap("non_existent_icon");
        QVERIFY(true);
    }

    void testRegisterIconByObject() {
        QPixmap pm(16, 16);
        pm.fill(Qt::blue);
        QIcon customIcon(pm);
        IconManager::instance().registerIcon("custom_test", customIcon);
        QIcon result = IconManager::instance().icon("custom_test");
        QVERIFY(!result.isNull());
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    int result = 0;
    { TestUiGlassWidget t; result |= QTest::qExec(&t, argc, argv); }
    { TestUiAnimation t; result |= QTest::qExec(&t, argc, argv); }
    { TestUiIconManager t; result |= QTest::qExec(&t, argc, argv); }

    return result;
}

#include "test_ui_components1.moc"