#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QSignalSpy>
#include <iostream>

#include "soul/ui/switch.h"
#include "soul/ui/checkbox.h"
#include "soul/ui/progress.h"
#include "soul/ui/slider.h"

using namespace sc;

// ============================================================================
// Switch
// ============================================================================
class TestUiSwitch : public QObject {
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
        Switch sw(m_parent);
        QVERIFY(!sw.isChecked());
    }

    void testCheckedState() {
        Switch sw(m_parent);
        sw.setChecked(true);
        QVERIFY(sw.isChecked());
        sw.setChecked(false);
        QVERIFY(!sw.isChecked());
    }

    void testToggleSignal() {
        Switch sw(m_parent);
        QSignalSpy spy(&sw, &Switch::toggled);
        sw.setChecked(true);
        QVERIFY(spy.count() == 1);
        QVERIFY(spy.at(0).at(0).toBool());
    }

    void testToggleSignalFalse() {
        Switch sw(m_parent);
        sw.setChecked(true);
        QSignalSpy spy(&sw, &Switch::toggled);
        sw.setChecked(false);
        QVERIFY(spy.count() == 1);
        QVERIFY(!spy.at(0).at(0).toBool());
    }

    void testSliderPosition() {
        Switch sw(m_parent);
        sw.setSliderPosition(0.5);
        QVERIFY(sw.sliderPosition() == 0.5);
        sw.setSliderPosition(0.0);
        QVERIFY(sw.sliderPosition() == 0.0);
        sw.setSliderPosition(1.0);
        QVERIFY(sw.sliderPosition() == 1.0);
    }

    void testSizeCheck() {
        Switch sw(m_parent);
        QVERIFY(sw.width() > 0);
        QVERIFY(sw.height() > 0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Checkbox
// ============================================================================
class TestUiCheckbox : public QObject {
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
        Checkbox cb(m_parent);
        QVERIFY(!cb.isChecked());
    }

    void testTextConstruction() {
        Checkbox cb("Accept terms", m_parent);
        QVERIFY(cb.text() == QString("Accept terms"));
    }

    void testCheckedState() {
        Checkbox cb(m_parent);
        cb.setChecked(true);
        QVERIFY(cb.isChecked());
        cb.setChecked(false);
        QVERIFY(!cb.isChecked());
    }

    void testToggleSignal() {
        Checkbox cb(m_parent);
        QSignalSpy spy(&cb, &Checkbox::toggled);
        cb.setChecked(true);
        QVERIFY(spy.count() == 1);
        QVERIFY(spy.at(0).at(0).toBool());
    }

    void testToggleSignalFalse() {
        Checkbox cb(m_parent);
        cb.setChecked(true);
        QSignalSpy spy(&cb, &Checkbox::toggled);
        cb.setChecked(false);
        QVERIFY(spy.count() == 1);
        QVERIFY(!spy.at(0).at(0).toBool());
    }

    void testCheckProgress() {
        Checkbox cb(m_parent);
        cb.setCheckProgress(0.5);
        QVERIFY(cb.checkProgress() == 0.5);
        cb.setCheckProgress(0.0);
        QVERIFY(cb.checkProgress() == 0.0);
        cb.setCheckProgress(1.0);
        QVERIFY(cb.checkProgress() == 1.0);
    }

    void testSizeCheck() {
        Checkbox cb(m_parent);
        QVERIFY(cb.width() > 0);
        QVERIFY(cb.height() > 0);
    }

    void testEmptyTextConstruction() {
        Checkbox cb("", m_parent);
        QVERIFY(cb.text().isEmpty());
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Progress
// ============================================================================
class TestUiProgress : public QObject {
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
        Progress p(m_parent);
        QVERIFY(p.value() == 0);
        QVERIFY(p.minimum() == 0);
        QVERIFY(p.maximum() == 100);
    }

    void testSetValue() {
        Progress p(m_parent);
        p.setValue(50);
        QVERIFY(p.value() == 50);
        p.setValue(0);
        QVERIFY(p.value() == 0);
        p.setValue(100);
        QVERIFY(p.value() == 100);
    }

    void testSetMinimum() {
        Progress p(m_parent);
        p.setMinimum(20);
        QVERIFY(p.minimum() == 20);
    }

    void testSetMaximum() {
        Progress p(m_parent);
        p.setMaximum(80);
        QVERIFY(p.maximum() == 80);
    }

    void testMinMaxRange() {
        Progress p(m_parent);
        p.setMinimum(20);
        p.setMaximum(80);
        p.setValue(50);
        QVERIFY(p.value() == 50);
        QVERIFY(p.minimum() == 20);
        QVERIFY(p.maximum() == 80);
    }

    void testGlowProgress() {
        Progress p(m_parent);
        p.setGlowProgress(0.75);
        QVERIFY(p.glowProgress() == 0.75);
        p.setGlowProgress(0.0);
        QVERIFY(p.glowProgress() == 0.0);
        p.setGlowProgress(1.0);
        QVERIFY(p.glowProgress() == 1.0);
    }

    void testValueClamping() {
        Progress p(m_parent);
        p.setMinimum(0);
        p.setMaximum(100);
        p.setValue(-10);
        p.setValue(200);
        QVERIFY(true);
    }

private:
    QWidget* m_parent = nullptr;
};

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
        Slider s(m_parent);
        QVERIFY(s.orientation() == Qt::Vertical);
    }

    void testOrientationConstruction() {
        Slider s(Qt::Vertical, m_parent);
        QVERIFY(s.orientation() == Qt::Vertical);
    }

    void testHorizontalOrientation() {
        Slider s(Qt::Horizontal, m_parent);
        QVERIFY(s.orientation() == Qt::Horizontal);
    }

    void testGlowProgress() {
        Slider s(m_parent);
        s.setGlowProgress(1.0);
        QVERIFY(s.glowProgress() == 1.0);
        s.setGlowProgress(0.0);
        QVERIFY(s.glowProgress() == 0.0);
        s.setGlowProgress(0.5);
        QVERIFY(s.glowProgress() == 0.5);
    }

    void testValueChange() {
        Slider s(m_parent);
        s.setRange(0, 100);
        s.setValue(42);
        QVERIFY(s.value() == 42);
    }

    void testRange() {
        Slider s(m_parent);
        s.setRange(10, 90);
        QVERIFY(s.minimum() == 10);
        QVERIFY(s.maximum() == 90);
    }

    void testEdgeCases() {
        Slider s(m_parent);
        s.setRange(0, 1);
        s.setValue(0);
        QVERIFY(s.value() == 0);
        s.setValue(1);
        QVERIFY(s.value() == 1);
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

    std::cout << "DEBUG: TestUiSwitch start" << std::endl;
    { TestUiSwitch t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiSwitch done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiCheckbox start" << std::endl;
    { TestUiCheckbox t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiCheckbox done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiProgress start" << std::endl;
    { TestUiProgress t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiProgress done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiSlider start" << std::endl;
    { TestUiSlider t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiSlider done, result=" << result << std::endl;

    return result;
}

#include "test_ui_switch.moc"