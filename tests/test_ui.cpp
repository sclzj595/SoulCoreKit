#include <QTest>
#include <QApplication>
#include "soul/ui/theme.h"
#include "soul/ui/style.h"
#include "soul/ui/base_widget.h"

using namespace sc;
using namespace sc::ui;

class TestTheme : public QObject {
    Q_OBJECT

private slots:
    void testThemeModeEnum();
    void testInitShutdown();
    void testSetMode();
    void testMode();
    void testApplyToApp();
};

void TestTheme::testThemeModeEnum() {
    ThemeMode light = ThemeMode::Light;
    ThemeMode dark = ThemeMode::Dark;
    ThemeMode system = ThemeMode::System;
    
    QVERIFY(light != dark);
    QVERIFY(dark != system);
    QVERIFY(light != system);
}

void TestTheme::testInitShutdown() {
    Theme::instance().init();
    Theme::instance().shutdown();
    QVERIFY(true);
}

void TestTheme::testSetMode() {
    Theme::instance().init();
    Theme::instance().setMode(ThemeMode::Dark);
    QCOMPARE(Theme::instance().mode(), ThemeMode::Dark);
    Theme::instance().setMode(ThemeMode::Light);
    QCOMPARE(Theme::instance().mode(), ThemeMode::Light);
    Theme::instance().shutdown();
}

void TestTheme::testMode() {
    Theme::instance().init();
    Theme::instance().setMode(ThemeMode::System);
    QCOMPARE(Theme::instance().mode(), ThemeMode::System);
    Theme::instance().shutdown();
}

void TestTheme::testApplyToApp() {
    Theme::instance().init();
    Theme::instance().setMode(ThemeMode::Light);
    Theme::instance().applyToApp();
    QVERIFY(true);
    Theme::instance().shutdown();
}

class TestStyle : public QObject {
    Q_OBJECT

private slots:
    void testStyleCreation();
};

void TestStyle::testStyleCreation() {
    Style style;
    QColor c = style.color(ColorRole::Primary);
    QVERIFY(c.isValid());
}

class TestBaseWidget : public QObject {
    Q_OBJECT

private slots:
    void testWidgetCreation();
    void testWidgetParent();
};

void TestBaseWidget::testWidgetCreation() {
    BaseWidget widget;
    QVERIFY(true);
}

void TestBaseWidget::testWidgetParent() {
    BaseWidget parent;
    BaseWidget child(&parent);
    QCOMPARE(child.parentWidget(), &parent);
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    int result = 0;

    {
        TestTheme t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestStyle t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestBaseWidget t;
        result |= QTest::qExec(&t, argc, argv);
    }

    return result;
}

#include "test_ui.moc"