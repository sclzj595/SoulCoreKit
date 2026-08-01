#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QSignalSpy>
#include <iostream>

#include "soul/ui/checkbox.h"
#include "soul/ui/card.h"
#include "soul/ui/badge.h"
#include "soul/ui/input.h"
#include "soul/ui/progress.h"

using namespace sc;

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
        QVERIFY(cb.checkProgress() >= 0.0);
    }

    void testConstructionWithText() {
        Checkbox cb("Accept", m_parent);
        QVERIFY(cb.text() == QString("Accept"));
        QVERIFY(!cb.isChecked());
    }

    void testSetChecked() {
        Checkbox cb(m_parent);
        cb.setChecked(true);
        QVERIFY(cb.isChecked());
        cb.setChecked(false);
        QVERIFY(!cb.isChecked());
    }

    void testCheckProgress() {
        Checkbox cb(m_parent);
        cb.setCheckProgress(0.5);
        QVERIFY(cb.checkProgress() == 0.5);
        cb.setCheckProgress(1.0);
        QVERIFY(cb.checkProgress() == 1.0);
        cb.setCheckProgress(0.0);
        QVERIFY(cb.checkProgress() == 0.0);
    }

    void testToggleSignal() {
        Checkbox cb(m_parent);
        QSignalSpy spy(&cb, &Checkbox::toggled);
        cb.setChecked(true);
        QVERIFY(spy.count() == 1);
        QVERIFY(spy[0][0].toBool() == true);
        cb.setChecked(false);
        QVERIFY(spy.count() == 2);
        QVERIFY(spy[1][0].toBool() == false);
    }

    void testNoDuplicateToggle() {
        Checkbox cb(m_parent);
        cb.setChecked(true);
        QSignalSpy spy(&cb, &Checkbox::toggled);
        cb.setChecked(true);
        QVERIFY(spy.count() == 0);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Card
// ============================================================================
class TestUiCard : public QObject {
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
        Card card(m_parent);
        QVERIFY(card.borderRadius() >= 0);
        QVERIFY(card.opacity() >= 0.0);
        QVERIFY(card.tintColor().isValid());
        QVERIFY(card.blurRadius() >= 0);
    }

    void testBorderRadius() {
        Card card(m_parent);
        card.setBorderRadius(16);
        QVERIFY(card.borderRadius() == 16);
        card.setBorderRadius(0);
        QVERIFY(card.borderRadius() == 0);
        card.setBorderRadius(32);
        QVERIFY(card.borderRadius() == 32);
    }

    void testOpacity() {
        Card card(m_parent);
        card.setOpacity(0.65);
        QVERIFY(card.opacity() == 0.65);
        card.setOpacity(1.0);
        QVERIFY(card.opacity() == 1.0);
        card.setOpacity(0.0);
        QVERIFY(card.opacity() == 0.0);
    }

    void testHoverEnabled() {
        Card card(m_parent);
        QVERIFY(card.isHoverEnabled());
        card.setHoverEnabled(false);
        QVERIFY(!card.isHoverEnabled());
        card.setHoverEnabled(true);
        QVERIFY(card.isHoverEnabled());
    }

    void testTintColor() {
        Card card(m_parent);
        card.setTintColor(QColor(255, 255, 255, 128));
        QVERIFY(card.tintColor() == QColor(255, 255, 255, 128));
        card.setTintColor(QColor(0, 0, 0, 64));
        QVERIFY(card.tintColor() == QColor(0, 0, 0, 64));
    }

    void testBlurRadius() {
        Card card(m_parent);
        card.setBlurRadius(20);
        QVERIFY(card.blurRadius() == 20);
        card.setBlurRadius(0);
        QVERIFY(card.blurRadius() == 0);
    }

    void testContentLayout() {
        Card card(m_parent);
        QLayout* layout = card.contentLayout();
        QVERIFY(layout != nullptr);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Badge
// ============================================================================
class TestUiBadge : public QObject {
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
        Badge badge(m_parent);
        QVERIFY(badge.count() == 0);
    }

    void testSetCount() {
        Badge badge(m_parent);
        badge.setCount(5);
        QVERIFY(badge.count() == 5);
        badge.setCount(0);
        QVERIFY(badge.count() == 0);
        badge.setCount(99);
        QVERIFY(badge.count() == 99);
    }

    void testNegativeCount() {
        Badge badge(m_parent);
        badge.setCount(-1);
        QVERIFY(badge.count() == -1);
    }

    void testVisibility() {
        Badge badge(m_parent);
        badge.setCount(5);
        QVERIFY(badge.isVisible());
        badge.setVisible(false);
        QVERIFY(!badge.isVisible());
        badge.setVisible(true);
        QVERIFY(badge.isVisible());
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Input
// ============================================================================
class TestUiInput : public QObject {
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
        Input input(m_parent);
        QVERIFY(static_cast<int>(input.inputType()) == static_cast<int>(Input::Normal));
        QVERIFY(!input.hasError());
        QVERIFY(input.errorMessage().isEmpty());
    }

    void testConstructionWithPlaceholder() {
        Input input("Search here...", m_parent);
        QVERIFY(input.placeholderText() == QString("Search here..."));
    }

    void testInputType() {
        Input input(m_parent);
        input.setInputType(Input::Password);
        QVERIFY(static_cast<int>(input.inputType()) == static_cast<int>(Input::Password));
        input.setInputType(Input::Search);
        QVERIFY(static_cast<int>(input.inputType()) == static_cast<int>(Input::Search));
        input.setInputType(Input::Email);
        QVERIFY(static_cast<int>(input.inputType()) == static_cast<int>(Input::Email));
        input.setInputType(Input::Normal);
        QVERIFY(static_cast<int>(input.inputType()) == static_cast<int>(Input::Normal));
    }

    void testErrorState() {
        Input input(m_parent);
        input.setError(true);
        QVERIFY(input.hasError());
        input.setError(false);
        QVERIFY(!input.hasError());
    }

    void testErrorMessage() {
        Input input(m_parent);
        input.setErrorMessage("Invalid input");
        QVERIFY(input.errorMessage() == QString("Invalid input"));
        input.setErrorMessage("");
        QVERIFY(input.errorMessage().isEmpty());
    }

    void testTextInput() {
        Input input(m_parent);
        input.setText("Hello");
        QVERIFY(input.text() == QString("Hello"));
        input.clear();
        QVERIFY(input.text().isEmpty());
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
        Progress prog(m_parent);
        QVERIFY(prog.value() == 0);
        QVERIFY(prog.minimum() == 0);
        QVERIFY(prog.maximum() == 100);
    }

    void testSetValue() {
        Progress prog(m_parent);
        prog.setValue(50);
        QVERIFY(prog.value() == 50);
        prog.setValue(0);
        QVERIFY(prog.value() == 0);
        prog.setValue(100);
        QVERIFY(prog.value() == 100);
    }

    void testSetMinimum() {
        Progress prog(m_parent);
        prog.setMinimum(10);
        QVERIFY(prog.minimum() == 10);
        prog.setMinimum(0);
        QVERIFY(prog.minimum() == 0);
    }

    void testSetMaximum() {
        Progress prog(m_parent);
        prog.setMaximum(200);
        QVERIFY(prog.maximum() == 200);
        prog.setMaximum(100);
        QVERIFY(prog.maximum() == 100);
    }

    void testGlowProgress() {
        Progress prog(m_parent);
        prog.setGlowProgress(0.5);
        QVERIFY(prog.glowProgress() == 0.5);
        prog.setGlowProgress(0.0);
        QVERIFY(prog.glowProgress() == 0.0);
        prog.setGlowProgress(1.0);
        QVERIFY(prog.glowProgress() == 1.0);
    }

    void testValueClamping() {
        Progress prog(m_parent);
        prog.setValue(150);
        QVERIFY(prog.value() >= prog.minimum());
        QVERIFY(prog.value() <= prog.maximum());
        prog.setValue(-50);
        QVERIFY(prog.value() >= prog.minimum());
        QVERIFY(prog.value() <= prog.maximum());
    }

private:
    QWidget* m_parent = nullptr;
};

int main(int argc, char* argv[]) {
    std::cout << "DEBUG: main start" << std::endl;
    QApplication app(argc, argv);
    std::cout << "DEBUG: QApplication created" << std::endl;

    int result = 0;
    std::cout << "DEBUG: TestUiCheckbox start" << std::endl;
    { TestUiCheckbox t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiCheckbox done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiCard start" << std::endl;
    { TestUiCard t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiCard done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiBadge start" << std::endl;
    { TestUiBadge t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiBadge done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiInput start" << std::endl;
    { TestUiInput t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiInput done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiProgress start" << std::endl;
    { TestUiProgress t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiProgress done, result=" << result << std::endl;

    return result;
}

#include "test_ui_components3.moc"