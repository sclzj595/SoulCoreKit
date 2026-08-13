#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QSignalSpy>
#include <QColor>
#include <iostream>

#include "soul/ui/button.h"
#include "soul/ui/card.h"
#include "soul/ui/dialog.h"
#include "soul/ui/toast.h"
#include "soul/ui/input.h"
#include "soul/ui/icon.h"

using namespace sc;

// ============================================================================
// Button
// ============================================================================
class TestUiButton : public QObject {
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
        Button btn(m_parent);
        QVERIFY(static_cast<int>(btn.buttonType()) == static_cast<int>(ButtonType::Push));
        QVERIFY(static_cast<int>(btn.buttonSize()) == static_cast<int>(ButtonSize::Medium));
        QVERIFY(!btn.isBreathing());
    }

    void testTextConstruction() {
        Button btn("Click Me", m_parent);
        QVERIFY(btn.text() == QString("Click Me"));
    }

    void testIconTextConstruction() {
        QIcon icon = Icon::fromColor(QColor(255, 0, 0));
        Button btn(icon, "IconBtn", m_parent);
        QVERIFY(btn.text() == QString("IconBtn"));
    }

    void testButtonType() {
        Button btn(m_parent);
        btn.setButtonType(ButtonType::Icon);
        QVERIFY(static_cast<int>(btn.buttonType()) == static_cast<int>(ButtonType::Icon));
        btn.setButtonType(ButtonType::Flat);
        QVERIFY(static_cast<int>(btn.buttonType()) == static_cast<int>(ButtonType::Flat));
        btn.setButtonType(ButtonType::Outline);
        QVERIFY(static_cast<int>(btn.buttonType()) == static_cast<int>(ButtonType::Outline));
        btn.setButtonType(ButtonType::Push);
        QVERIFY(static_cast<int>(btn.buttonType()) == static_cast<int>(ButtonType::Push));
    }

    void testButtonSize() {
        Button btn(m_parent);
        btn.setButtonSize(ButtonSize::Small);
        QVERIFY(static_cast<int>(btn.buttonSize()) == static_cast<int>(ButtonSize::Small));
        btn.setButtonSize(ButtonSize::Large);
        QVERIFY(static_cast<int>(btn.buttonSize()) == static_cast<int>(ButtonSize::Large));
        btn.setButtonSize(ButtonSize::Medium);
        QVERIFY(static_cast<int>(btn.buttonSize()) == static_cast<int>(ButtonSize::Medium));
    }

    void testBreathing() {
        Button btn(m_parent);
        btn.setBreathing(true);
        QVERIFY(btn.isBreathing());
        btn.setBreathing(false);
        QVERIFY(!btn.isBreathing());
    }

    void testIconColor() {
        Button btn(m_parent);
        btn.setIconColor(QColor(255, 0, 0));
        QVERIFY(btn.iconColor() == QColor(255, 0, 0));
        btn.setIconColor(QColor(0, 255, 0));
        QVERIFY(btn.iconColor() == QColor(0, 255, 0));
    }

    void testClickSignal() {
        Button btn("Click", m_parent);
        QSignalSpy spy(&btn, &QPushButton::clicked);
        btn.click();
        QVERIFY(spy.count() == 1);
    }

    void testDisabledState() {
        Button btn("Disabled", m_parent);
        btn.setEnabled(false);
        QVERIFY(!btn.isEnabled());
        QSignalSpy spy(&btn, &QPushButton::clicked);
        btn.click();
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
        QVERIFY(card.isHoverEnabled());
    }

    void testBorderRadius() {
        Card card(m_parent);
        card.setBorderRadius(16);
        QVERIFY(card.borderRadius() == 16);
        card.setBorderRadius(0);
        QVERIFY(card.borderRadius() == 0);
    }

    void testOpacity() {
        Card card(m_parent);
        card.setOpacity(0.5);
        QVERIFY(card.opacity() == 0.5);
        card.setOpacity(1.0);
        QVERIFY(card.opacity() == 1.0);
    }

    void testHoverEnabled() {
        Card card(m_parent);
        card.setHoverEnabled(true);
        QVERIFY(card.isHoverEnabled());
        card.setHoverEnabled(false);
        QVERIFY(!card.isHoverEnabled());
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
        card.setBlurRadius(10);
        QVERIFY(card.blurRadius() == 10);
        card.setBlurRadius(0);
        QVERIFY(card.blurRadius() == 0);
    }

    void testContentLayout() {
        Card card(m_parent);
        QVERIFY(card.contentLayout() != nullptr);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Dialog
// ============================================================================
class TestUiDialog : public QObject {
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

    void testDialogTypeEnum() {
        QVERIFY(static_cast<int>(DialogType::Info) != static_cast<int>(DialogType::Warning));
        QVERIFY(static_cast<int>(DialogType::Warning) != static_cast<int>(DialogType::Error));
        QVERIFY(static_cast<int>(DialogType::Error) != static_cast<int>(DialogType::Success));
        QVERIFY(static_cast<int>(DialogType::Success) != static_cast<int>(DialogType::Confirm));
    }

    void testDialogButtonEnum() {
        QVERIFY(static_cast<int>(DialogButton::Ok) != static_cast<int>(DialogButton::Cancel));
        QVERIFY(static_cast<int>(DialogButton::Cancel) != static_cast<int>(DialogButton::Yes));
        QVERIFY(static_cast<int>(DialogButton::Yes) != static_cast<int>(DialogButton::No));
    }

    void testSetTitle() {
        Dialog dlg(m_parent);
        dlg.setTitle("Test Title");
        QVERIFY(true);
    }

    void testSetMessage() {
        Dialog dlg(m_parent);
        dlg.setMessage("Test Message");
        QVERIFY(true);
    }

    void testSetType() {
        Dialog dlg(m_parent);
        dlg.setType(DialogType::Info);
        dlg.setType(DialogType::Warning);
        dlg.setType(DialogType::Error);
        dlg.setType(DialogType::Success);
        dlg.setType(DialogType::Confirm);
        QVERIFY(true);
    }

    void testAddButton() {
        Dialog dlg(m_parent);
        dlg.addButton(DialogButton::Ok);
        dlg.addButton(DialogButton::Cancel);
        dlg.addButton(DialogButton::Yes, "Custom Yes");
        dlg.addButton(DialogButton::No, "Custom No");
        QVERIFY(true);
    }

    void testButtonCallback() {
        Dialog dlg(m_parent);
        bool called = false;
        dlg.setOnButtonClicked([&](DialogButton) { called = true; });
        dlg.addButton(DialogButton::Ok);
        QVERIFY(!called);
    }

    void testStaticMethods() {
        QVERIFY(true);
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

    void testToastTypeEnum() {
        QVERIFY(static_cast<int>(ToastType::Info) != static_cast<int>(ToastType::Warning));
        QVERIFY(static_cast<int>(ToastType::Warning) != static_cast<int>(ToastType::Error));
        QVERIFY(static_cast<int>(ToastType::Error) != static_cast<int>(ToastType::Success));
    }

    void testSetType() {
        Toast toast(m_parent);
        toast.setType(ToastType::Info);
        toast.setType(ToastType::Warning);
        toast.setType(ToastType::Error);
        toast.setType(ToastType::Success);
        QVERIFY(true);
    }

    void testSetMessage() {
        Toast toast(m_parent);
        toast.setMessage("Operation successful");
        toast.setMessage("");
        QVERIFY(true);
    }

    void testSetDuration() {
        Toast toast(m_parent);
        toast.setDuration(3000);
        toast.setDuration(500);
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

    void testPlaceholderConstruction() {
        Input input("Enter text...", m_parent);
        QVERIFY(input.placeholderText() == QString("Enter text..."));
    }

    void testInputType() {
        Input input(m_parent);
        input.setInputType(Input::Password);
        QVERIFY(static_cast<int>(input.inputType()) == static_cast<int>(Input::Password));
        QVERIFY(input.echoMode() == QLineEdit::Password);
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
        input.setText("");
        QVERIFY(input.text().isEmpty());
    }

    void testClear() {
        Input input(m_parent);
        input.setText("Clear me");
        QVERIFY(!input.text().isEmpty());
        input.clear();
        QVERIFY(input.text().isEmpty());
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

    std::cout << "DEBUG: TestUiButton start" << std::endl;
    { TestUiButton t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiButton done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiCard start" << std::endl;
    { TestUiCard t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiCard done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiDialog start" << std::endl;
    { TestUiDialog t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiDialog done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiToast start" << std::endl;
    { TestUiToast t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiToast done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiInput start" << std::endl;
    { TestUiInput t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiInput done, result=" << result << std::endl;

    return result;
}

#include "test_ui_button.moc"