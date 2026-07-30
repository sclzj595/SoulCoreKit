#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include <QStackedWidget>

#include "soul/ui/theme.h"
#include "soul/ui/style.h"
#include "soul/ui/base_widget.h"
#include "soul/ui/button.h"
#include "soul/ui/card.h"
#include "soul/ui/dialog.h"
#include "soul/ui/toast.h"
#include "soul/ui/input.h"
#include "soul/ui/switch.h"
#include "soul/ui/checkbox.h"
#include "soul/ui/progress.h"
#include "soul/ui/slider.h"
#include "soul/ui/avatar.h"
#include "soul/ui/badge.h"
#include "soul/ui/loading.h"
#include "soul/ui/spinner.h"
#include "soul/ui/dropdown.h"
#include "soul/ui/tab_bar.h"
#include "soul/ui/tool_tip.h"
#include "soul/ui/empty_widget.h"
#include "soul/ui/scroll_bar.h"
#include "soul/ui/window.h"
#include "soul/ui/icon.h"
#include "soul/ui/navigation.h"
#include "soul/ui/page.h"
#include "soul/ui/base_view.h"
#include "soul/ui/sidebar.h"

using namespace sc;
using namespace sc::ui;

// Theme
class TestTheme : public QObject {
    Q_OBJECT
private slots:
    void testThemeModeEnum() { QVERIFY(ThemeMode::Light != ThemeMode::Dark); }
    void testInitShutdown() { Theme::instance().init(); Theme::instance().shutdown(); QVERIFY(true); }
    void testSetMode() {
        Theme::instance().init();
        Theme::instance().setMode(ThemeMode::Dark);
        QCOMPARE(Theme::instance().mode(), ThemeMode::Dark);
        Theme::instance().shutdown();
    }
    void testApplyToApp() {
        Theme::instance().init();
        Theme::instance().setMode(ThemeMode::Light);
        Theme::instance().applyToApp();
        Theme::instance().shutdown();
    }
};

// Style
class TestStyle : public QObject {
    Q_OBJECT
private slots:
    void testStyleCreation() { Style s; QVERIFY(s.color(ColorRole::Primary).isValid()); }
    void testAllColorRoles() {
        Style s;
        QList<ColorRole> roles = {ColorRole::Primary, ColorRole::Secondary, ColorRole::Background, ColorRole::Surface, ColorRole::Error, ColorRole::Success};
        for (auto r : roles) QVERIFY(s.color(r).isValid());
    }
    void testCornerRadius() { Style s; s.setCornerRadius(CornerRadius::Medium, 12); QCOMPARE(s.cornerRadius(CornerRadius::Medium), 12); }
    void testSpacing() { Style s; s.setSpacing(Spacing::Medium, 16); QCOMPARE(s.spacing(Spacing::Medium), 16); }
    void testFont() { Style s; QVERIFY(s.font().family().size() > 0); QVERIFY(s.boldFont().bold()); }
};

// BaseWidget
class TestBaseWidget : public QObject {
    Q_OBJECT
private slots:
    void testWidgetCreation() { BaseWidget w; QVERIFY(true); }
    void testWidgetParent() { BaseWidget p; BaseWidget c(&p); QCOMPARE(c.parentWidget(), &p); }
    void testWidgetVisible() { BaseWidget w; w.show(); QVERIFY(w.isVisible()); w.hide(); QVERIFY(!w.isVisible()); }
    void testWidgetEnabled() { BaseWidget w; QVERIFY(w.isEnabled()); w.setEnabled(false); QVERIFY(!w.isEnabled()); }
};

// Button
class TestButton : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Button btn;
        QCOMPARE(btn.buttonType(), ButtonType::Push);
        QCOMPARE(btn.buttonSize(), ButtonSize::Medium);
        QVERIFY(!btn.isBreathing());
    }
    void testTextConstruction() {
        Button btn("Click Me");
        QCOMPARE(btn.text(), QString("Click Me"));
    }
    void testButtonType() {
        Button btn;
        btn.setButtonType(ButtonType::Icon);
        QCOMPARE(btn.buttonType(), ButtonType::Icon);
        btn.setButtonType(ButtonType::Flat);
        QCOMPARE(btn.buttonType(), ButtonType::Flat);
        btn.setButtonType(ButtonType::Outline);
        QCOMPARE(btn.buttonType(), ButtonType::Outline);
        btn.setButtonType(ButtonType::Push);
        QCOMPARE(btn.buttonType(), ButtonType::Push);
    }
    void testButtonSize() {
        Button btn;
        btn.setButtonSize(ButtonSize::Small);
        QCOMPARE(btn.buttonSize(), ButtonSize::Small);
        btn.setButtonSize(ButtonSize::Large);
        QCOMPARE(btn.buttonSize(), ButtonSize::Large);
        btn.setButtonSize(ButtonSize::Medium);
        QCOMPARE(btn.buttonSize(), ButtonSize::Medium);
    }
    void testBreathing() {
        Button btn;
        btn.setBreathing(true);
        QVERIFY(btn.isBreathing());
        btn.setBreathing(false);
        QVERIFY(!btn.isBreathing());
    }
    void testIconColor() {
        Button btn;
        btn.setIconColor(QColor(255, 0, 0));
        QCOMPARE(btn.iconColor(), QColor(255, 0, 0));
    }
    void testClickSignal() {
        Button btn("Click");
        QSignalSpy spy(&btn, &QPushButton::clicked);
        btn.click();
        QCOMPARE(spy.count(), 1);
    }
    void testDisabledState() {
        Button btn("Disabled");
        btn.setEnabled(false);
        QVERIFY(!btn.isEnabled());
        QSignalSpy spy(&btn, &QPushButton::clicked);
        btn.click();
        QCOMPARE(spy.count(), 0);
    }
};

// Card
class TestCard : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Card card;
        QVERIFY(card.borderRadius() >= 0);
        QVERIFY(!card.isHoverEnabled());
    }
    void testBorderRadius() {
        Card card;
        card.setBorderRadius(16);
        QCOMPARE(card.borderRadius(), 16);
    }
    void testOpacity() {
        Card card;
        card.setOpacity(0.5);
        QCOMPARE(card.opacity(), 0.5);
    }
    void testHoverEnabled() {
        Card card;
        card.setHoverEnabled(true);
        QVERIFY(card.isHoverEnabled());
    }
    void testTintColor() {
        Card card;
        card.setTintColor(QColor(255, 255, 255, 128));
        QCOMPARE(card.tintColor(), QColor(255, 255, 255, 128));
    }
    void testBlurRadius() {
        Card card;
        card.setBlurRadius(10);
        QCOMPARE(card.blurRadius(), 10);
    }
    void testContentLayout() {
        Card card;
        QVERIFY(card.contentLayout() != nullptr);
    }
};

// Dialog
class TestDialog : public QObject {
    Q_OBJECT
private slots:
    void testDialogTypeEnum() {
        QVERIFY(DialogType::Info != DialogType::Warning);
        QVERIFY(DialogType::Warning != DialogType::Error);
        QVERIFY(DialogType::Error != DialogType::Success);
        QVERIFY(DialogType::Success != DialogType::Confirm);
    }
    void testDialogButtonEnum() {
        QVERIFY(DialogButton::Ok != DialogButton::Cancel);
        QVERIFY(DialogButton::Cancel != DialogButton::Yes);
        QVERIFY(DialogButton::Yes != DialogButton::No);
    }
    void testSetTitle() {
        Dialog dlg;
        dlg.setTitle("Test Title");
        QCOMPARE(dlg.windowTitle(), QString("Test Title"));
    }
    void testSetType() {
        Dialog dlg;
        dlg.setType(DialogType::Info);
        dlg.setType(DialogType::Warning);
        dlg.setType(DialogType::Error);
        dlg.setType(DialogType::Success);
        dlg.setType(DialogType::Confirm);
        QVERIFY(true);
    }
    void testAddButton() {
        Dialog dlg;
        dlg.addButton(DialogButton::Ok);
        dlg.addButton(DialogButton::Cancel);
        QVERIFY(true);
    }
    void testButtonCallback() {
        Dialog dlg;
        bool called = false;
        dlg.setOnButtonClicked([&](DialogButton) { called = true; });
        dlg.addButton(DialogButton::Ok);
        QVERIFY(!called);
    }
};

// Toast
class TestToast : public QObject {
    Q_OBJECT
private slots:
    void testToastTypeEnum() {
        QVERIFY(ToastType::Info != ToastType::Warning);
        QVERIFY(ToastType::Warning != ToastType::Error);
        QVERIFY(ToastType::Error != ToastType::Success);
    }
    void testSetType() {
        Toast toast;
        toast.setType(ToastType::Info);
        toast.setType(ToastType::Warning);
        QVERIFY(true);
    }
    void testSetMessage() {
        Toast toast;
        toast.setMessage("Operation successful");
        QVERIFY(true);
    }
    void testSetDuration() {
        Toast toast;
        toast.setDuration(3000);
        QVERIFY(true);
    }
    void testStaticMethods() {
        Toast::info("Info message");
        Toast::warning("Warning message");
        Toast::error("Error message");
        Toast::success("Success message");
        QVERIFY(true);
    }
};

// Input
class TestInput : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Input input;
        QCOMPARE(input.inputType(), Input::Normal);
        QVERIFY(!input.hasError());
        QVERIFY(input.errorMessage().isEmpty());
    }
    void testPlaceholderConstruction() {
        Input input("Enter text...");
        QCOMPARE(input.placeholderText(), QString("Enter text..."));
    }
    void testInputType() {
        Input input;
        input.setInputType(Input::Password);
        QCOMPARE(input.inputType(), Input::Password);
        QVERIFY(input.echoMode() == QLineEdit::Password);
        input.setInputType(Input::Normal);
        QCOMPARE(input.inputType(), Input::Normal);
    }
    void testErrorState() {
        Input input;
        input.setError(true);
        QVERIFY(input.hasError());
        input.setError(false);
        QVERIFY(!input.hasError());
    }
    void testErrorMessage() {
        Input input;
        input.setErrorMessage("Invalid input");
        QCOMPARE(input.errorMessage(), QString("Invalid input"));
    }
    void testTextInput() {
        Input input;
        input.setText("Hello");
        QCOMPARE(input.text(), QString("Hello"));
    }
    void testClear() {
        Input input;
        input.setText("Clear me");
        input.clear();
        QVERIFY(input.text().isEmpty());
    }
};

// Switch
class TestSwitch : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { Switch sw; QVERIFY(!sw.isChecked()); }
    void testCheckedState() {
        Switch sw;
        sw.setChecked(true);
        QVERIFY(sw.isChecked());
        sw.setChecked(false);
        QVERIFY(!sw.isChecked());
    }
    void testToggleSignal() {
        Switch sw;
        QSignalSpy spy(&sw, &Switch::toggled);
        sw.setChecked(true);
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy[0][0].toBool());
    }
    void testSliderPosition() {
        Switch sw;
        sw.setSliderPosition(0.5);
        QCOMPARE(sw.sliderPosition(), 0.5);
    }
    void testSizeCheck() {
        Switch sw;
        QVERIFY(sw.width() > 0);
        QVERIFY(sw.height() > 0);
    }
};

// Checkbox
class TestCheckbox : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { Checkbox cb; QVERIFY(!cb.isChecked()); }
    void testTextConstruction() {
        Checkbox cb("Accept terms");
        QCOMPARE(cb.text(), QString("Accept terms"));
    }
    void testCheckedState() {
        Checkbox cb;
        cb.setChecked(true);
        QVERIFY(cb.isChecked());
        cb.setChecked(false);
        QVERIFY(!cb.isChecked());
    }
    void testToggleSignal() {
        Checkbox cb;
        QSignalSpy spy(&cb, &Checkbox::toggled);
        cb.setChecked(true);
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy[0][0].toBool());
    }
    void testCheckProgress() {
        Checkbox cb;
        cb.setCheckProgress(0.5);
        QCOMPARE(cb.checkProgress(), 0.5);
    }
    void testSizeCheck() {
        Checkbox cb;
        QVERIFY(cb.width() > 0);
        QVERIFY(cb.height() > 0);
    }
};

// Progress
class TestProgress : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Progress p;
        QCOMPARE(p.value(), 0);
        QCOMPARE(p.minimum(), 0);
        QCOMPARE(p.maximum(), 100);
    }
    void testSetValue() {
        Progress p;
        p.setValue(50);
        QCOMPARE(p.value(), 50);
    }
    void testGlowProgress() {
        Progress p;
        p.setGlowProgress(0.75);
        QCOMPARE(p.glowProgress(), 0.75);
    }
    void testMinMax() {
        Progress p;
        p.setMinimum(20);
        p.setMaximum(80);
        p.setValue(50);
        QCOMPARE(p.value(), 50);
    }
};

// Slider
class TestSlider : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { Slider s; QCOMPARE(s.orientation(), Qt::Horizontal); }
    void testOrientationConstruction() { Slider s(Qt::Vertical); QCOMPARE(s.orientation(), Qt::Vertical); }
    void testGlowProgress() {
        Slider s;
        s.setGlowProgress(1.0);
        QCOMPARE(s.glowProgress(), 1.0);
    }
    void testValueChange() {
        Slider s;
        s.setRange(0, 100);
        s.setValue(42);
        QCOMPARE(s.value(), 42);
    }
    void testRange() {
        Slider s;
        s.setRange(10, 90);
        QCOMPARE(s.minimum(), 10);
        QCOMPARE(s.maximum(), 90);
    }
};

// Avatar
class TestAvatar : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Avatar av;
        QVERIFY(av.pixmap().isNull());
        QVERIFY(av.initials().isEmpty());
        QVERIFY(av.size() > 0);
    }
    void testPixmap() {
        Avatar av;
        QPixmap pm(32, 32);
        pm.fill(Qt::red);
        av.setPixmap(pm);
        QVERIFY(!av.pixmap().isNull());
        QCOMPARE(av.pixmap().size(), QSize(32, 32));
    }
    void testInitials() {
        Avatar av;
        av.setInitials("AB");
        QCOMPARE(av.initials(), QString("AB"));
    }
    void testSize() {
        Avatar av;
        av.setSize(64);
        QCOMPARE(av.size(), 64);
    }
    void testSizeCheck() {
        Avatar av;
        av.setSize(48);
        QVERIFY(av.width() >= 48);
        QVERIFY(av.height() >= 48);
    }
};

// Badge
class TestBadge : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Badge b;
        QCOMPARE(b.count(), 0);
        QVERIFY(b.isVisible());
    }
    void testCount() {
        Badge b;
        b.setCount(5);
        QCOMPARE(b.count(), 5);
    }
    void testVisibility() {
        Badge b;
        b.setVisible(false);
        QVERIFY(!b.isVisible());
    }
    void testSizeCheck() {
        Badge b;
        QVERIFY(b.width() > 0);
        QVERIFY(b.height() > 0);
    }
};

// Loading
class TestLoading : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Loading loading;
        QVERIFY(loading.text().isEmpty());
        QCOMPARE(loading.progress(), 0);
    }
    void testSetText() {
        Loading loading;
        loading.setText("Loading...");
        QCOMPARE(loading.text(), QString("Loading..."));
    }
    void testSetProgress() {
        Loading loading;
        loading.setProgress(50);
        QCOMPARE(loading.progress(), 50);
    }
    void testIndeterminate() {
        Loading loading;
        loading.setIndeterminate(true);
        QVERIFY(true);
    }
    void testShowProgress() {
        Loading loading;
        loading.showProgress(true);
        QVERIFY(true);
    }
    void testStaticMethods() {
        Loading::showGlobal("Loading...");
        Loading::updateGlobalProgress(50);
        Loading::hideGlobal();
        QVERIFY(true);
    }
};

// Spinner
class TestSpinnerUI : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { Spinner s; QCOMPARE(s.rotation(), 0.0); }
    void testRotation() {
        Spinner s;
        s.setRotation(90.0);
        QCOMPARE(s.rotation(), 90.0);
    }
    void testSizeCheck() { Spinner s; QVERIFY(s.width() > 0); QVERIFY(s.height() > 0); }
};

// Dropdown
class TestDropdown : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { Dropdown dd; QCOMPARE(dd.count(), 0); }
    void testAddItems() {
        Dropdown dd;
        dd.addItem("Option 1");
        dd.addItem("Option 2");
        QCOMPARE(dd.count(), 2);
    }
    void testCurrentIndex() {
        Dropdown dd;
        dd.addItem("A");
        dd.addItem("B");
        dd.setCurrentIndex(1);
        QCOMPARE(dd.currentIndex(), 1);
    }
};

// TabBar
class TestTabBar : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { TabBar tb; QCOMPARE(tb.count(), 0); }
    void testAddTab() {
        TabBar tb;
        tb.addTab("Tab 1");
        tb.addTab("Tab 2");
        QCOMPARE(tb.count(), 2);
        QCOMPARE(tb.tabText(0), QString("Tab 1"));
    }
    void testCurrentIndex() {
        TabBar tb;
        tb.addTab("Tab 1");
        tb.addTab("Tab 2");
        tb.setCurrentIndex(1);
        QCOMPARE(tb.currentIndex(), 1);
    }
};

// ToolTip
class TestToolTip : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { ToolTip tt; QVERIFY(tt.text().isEmpty()); }
    void testSetText() {
        ToolTip tt;
        tt.setText("Tooltip text");
        QCOMPARE(tt.text(), QString("Tooltip text"));
    }
    void testSizeCheck() {
        ToolTip tt;
        tt.setText("Hello");
        QVERIFY(tt.width() > 0);
        QVERIFY(tt.height() > 0);
    }
};

// EmptyWidget
class TestEmptyWidget : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { EmptyWidget ew; QVERIFY(true); }
    void testSetTitle() { EmptyWidget ew; ew.setTitle("No data"); QVERIFY(true); }
    void testSetSubtitle() { EmptyWidget ew; ew.setSubtitle("Please add some items"); QVERIFY(true); }
    void testSetIcon() {
        EmptyWidget ew;
        QPixmap pm(32, 32);
        pm.fill(Qt::blue);
        ew.setIcon(QIcon(pm));
        QVERIFY(true);
    }
    void testButtonText() { EmptyWidget ew; ew.setButtonText("Add Item"); QVERIFY(true); }
    void testShowButton() { EmptyWidget ew; ew.showButton(true); ew.showButton(false); QVERIFY(true); }
    void testButtonClickedSignal() {
        EmptyWidget ew;
        QSignalSpy spy(&ew, &EmptyWidget::buttonClicked);
        QCOMPARE(spy.count(), 0);
    }
};

// ScrollBar
class TestScrollBar : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { ScrollBar sb; QCOMPARE(sb.orientation(), Qt::Horizontal); }
    void testOrientationConstruction() { ScrollBar sb(Qt::Vertical); QCOMPARE(sb.orientation(), Qt::Vertical); }
    void testValueRange() {
        ScrollBar sb;
        sb.setRange(0, 100);
        sb.setValue(50);
        QCOMPARE(sb.value(), 50);
    }
};

// Window
class TestWindow : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Window win;
        QVERIFY(win.title().isEmpty());
        QVERIFY(!win.isFrameless());
        QVERIFY(!win.hasGlassEffect());
        QVERIFY(!win.hasGlowBorder());
    }
    void testTitle() {
        Window win;
        win.setTitle("My App");
        QCOMPARE(win.title(), QString("My App"));
    }
    void testFrameless() {
        Window win;
        win.setFrameless(true);
        QVERIFY(win.isFrameless());
    }
    void testGlassEffect() {
        Window win;
        win.setGlassEffect(true);
        QVERIFY(win.hasGlassEffect());
    }
    void testGlowBorder() {
        Window win;
        win.setGlowBorder(true);
        QVERIFY(win.hasGlowBorder());
    }
    void testBlurRadius() {
        Window win;
        win.setBlurRadius(20);
        QCOMPARE(win.blurRadius(), 20);
    }
    void testTintColor() {
        Window win;
        win.setTintColor(QColor(255, 255, 255, 128));
        QCOMPARE(win.tintColor(), QColor(255, 255, 255, 128));
    }
    void testWindowSignals() {
        Window win;
        QSignalSpy closedSpy(&win, &Window::windowClosed);
        QSignalSpy minSpy(&win, &Window::windowMinimized);
        QSignalSpy maxSpy(&win, &Window::windowMaximized);
        QCOMPARE(closedSpy.count(), 0);
        QCOMPARE(minSpy.count(), 0);
        QCOMPARE(maxSpy.count(), 0);
    }
};

// Icon
class TestIcon : public QObject {
    Q_OBJECT
private slots:
    void testFromColor() {
        QIcon icon = Icon::fromColor(QColor(255, 0, 0));
        QVERIFY(!icon.isNull());
    }
    void testAppIcon() { QVERIFY(true); }
    void testSetAppIcon() {
        QIcon customIcon = Icon::fromColor(QColor(0, 255, 0));
        Icon::setAppIcon(customIcon);
        QVERIFY(!Icon::appIcon().isNull());
    }
    void testFromColorDifferentSizes() {
        QVERIFY(!Icon::fromColor(QColor(0, 0, 255), 16).isNull());
        QVERIFY(!Icon::fromColor(QColor(0, 0, 255), 32).isNull());
        QVERIFY(!Icon::fromColor(QColor(0, 0, 255), 64).isNull());
    }
};

// Navigation
class TestNavigation : public QObject {
    Q_OBJECT
private slots:
    void testSetRootWidget() {
        QStackedWidget stack;
        Navigation::setRootWidget(&stack);
        QVERIFY(true);
    }
    void testPushPop() {
        QStackedWidget stack;
        Navigation::setRootWidget(&stack);
        Navigation::push(new QWidget(), "Page 1");
        QCOMPARE(Navigation::stackCount(), 1);
        QCOMPARE(Navigation::currentIndex(), 0);
        QCOMPARE(Navigation::currentTitle(), QString("Page 1"));
        Navigation::push(new QWidget(), "Page 2");
        QCOMPARE(Navigation::stackCount(), 2);
        Navigation::pop();
        QCOMPARE(Navigation::stackCount(), 1);
        QCOMPARE(Navigation::currentTitle(), QString("Page 1"));
    }
    void testPopToRoot() {
        QStackedWidget stack;
        Navigation::setRootWidget(&stack);
        Navigation::push(new QWidget(), "Page 1");
        Navigation::push(new QWidget(), "Page 2");
        Navigation::push(new QWidget(), "Page 3");
        QCOMPARE(Navigation::stackCount(), 3);
        Navigation::popToRoot();
        QCOMPARE(Navigation::stackCount(), 1);
    }
    void testStackCount() {
        QStackedWidget stack;
        Navigation::setRootWidget(&stack);
        QCOMPARE(Navigation::stackCount(), 0);
        Navigation::push(new QWidget(), "Page 1");
        QCOMPARE(Navigation::stackCount(), 1);
    }
    void testCurrentWidget() {
        QStackedWidget stack;
        Navigation::setRootWidget(&stack);
        auto* widget = new QWidget();
        Navigation::push(widget, "Test");
        QCOMPARE(Navigation::currentWidget(), widget);
    }
    void testReplace() {
        QStackedWidget stack;
        Navigation::setRootWidget(&stack);
        Navigation::push(new QWidget(), "Page 1");
        Navigation::replace(new QWidget(), "New Page");
        QCOMPARE(Navigation::stackCount(), 1);
        QCOMPARE(Navigation::currentTitle(), QString("New Page"));
    }
};

// Page
class TestPage : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        Page page;
        QVERIFY(page.pageTitle().isEmpty());
        QVERIFY(page.pageSubtitle().isEmpty());
    }
    void testPageTitle() {
        Page page;
        page.setPageTitle("Home");
        QCOMPARE(page.pageTitle(), QString("Home"));
    }
    void testPageSubtitle() {
        Page page;
        page.setPageSubtitle("Manage your preferences");
        QCOMPARE(page.pageSubtitle(), QString("Manage your preferences"));
    }
    void testLifecycle() {
        class TestPageDerived : public Page {
        public:
            bool entered = false, left = false, back = false;
            void onEnter() override { entered = true; }
            void onLeave() override { left = true; }
            void onBack() override { back = true; }
        };
        TestPageDerived page;
        page.onEnter(); QVERIFY(page.entered);
        page.onLeave(); QVERIFY(page.left);
        page.onBack(); QVERIFY(page.back);
    }
    void testSignals() {
        Page page;
        QSignalSpy enterSpy(&page, &Page::pageEnter);
        QSignalSpy leaveSpy(&page, &Page::pageLeave);
        QSignalSpy backSpy(&page, &Page::backPressed);
        QCOMPARE(enterSpy.count(), 0);
        QCOMPARE(leaveSpy.count(), 0);
        QCOMPARE(backSpy.count(), 0);
    }
};

// BaseView
class TestBaseView : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() { BaseView view; QVERIFY(view.title().isEmpty()); }
    void testShowHide() { BaseView view; view.show(); QVERIFY(view.isVisible()); view.hide(); QVERIFY(!view.isVisible()); }
    void testWidget() { BaseView view; QCOMPARE(view.widget(), &view); }
    void testSetTitle() { BaseView view; view.setTitle("My View"); QCOMPARE(view.title(), QString("My View")); }
    void testSetSize() { BaseView view; view.setSize(800, 600); QCOMPARE(view.size(), QSize(800, 600)); }
    void testSetPosition() { BaseView view; view.setPosition(100, 200); QCOMPARE(view.position(), QPoint(100, 200)); }
};

// SideBar
class TestSideBar : public QObject {
    Q_OBJECT
private slots:
    void testDefaultConstruction() {
        SideBar sb;
        QVERIFY(sb.activeItem().isEmpty());
        QVERIFY(!sb.isCollapsed());
        QVERIFY(sb.width() > 0);
    }
    void testAddItem() {
        SideBar sb;
        SideBarItem item;
        item.id = "home";
        item.text = "Home";
        sb.addItem(item);
        QVERIFY(true);
    }
    void testAddItems() {
        SideBar sb;
        QList<SideBarItem> items;
        SideBarItem item1; item1.id = "item1"; item1.text = "Item 1"; items.append(item1);
        SideBarItem item2; item2.id = "item2"; item2.text = "Item 2"; items.append(item2);
        sb.addItems(items);
        QVERIFY(true);
    }
    void testRemoveItem() {
        SideBar sb;
        SideBarItem item; item.id = "temp"; item.text = "Temp";
        sb.addItem(item);
        sb.removeItem("temp");
        QVERIFY(true);
    }
    void testActiveItem() {
        SideBar sb;
        SideBarItem item; item.id = "active"; item.text = "Active";
        sb.addItem(item);
        sb.setActiveItem("active");
        QCOMPARE(sb.activeItem(), QString("active"));
    }
    void testCollapsed() {
        SideBar sb;
        sb.setCollapsed(true);
        QVERIFY(sb.isCollapsed());
    }
    void testSetWidth() {
        SideBar sb;
        sb.setWidth(200);
        QCOMPARE(sb.width(), 200);
    }
    void testClearItems() {
        SideBar sb;
        SideBarItem item; item.id = "test"; item.text = "Test";
        sb.addItem(item);
        sb.clearItems();
        QVERIFY(true);
    }
    void testItemClickedSignal() {
        SideBar sb;
        QSignalSpy spy(&sb, &SideBar::itemClicked);
        QCOMPARE(spy.count(), 0);
    }
    void testSetItemIconColor() {
        SideBar sb;
        SideBarItem item; item.id = "colored"; item.text = "Colored";
        sb.addItem(item);
        sb.setItemIconColor("colored", QColor(255, 0, 0));
        QVERIFY(true);
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    int result = 0;

    { TestTheme t; result |= QTest::qExec(&t, argc, argv); }
    { TestStyle t; result |= QTest::qExec(&t, argc, argv); }
    { TestBaseWidget t; result |= QTest::qExec(&t, argc, argv); }
    return result;
}

#include "test_ui.moc"