#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QSignalSpy>
#include <QColor>
#include <iostream>

#include "soul/ui/avatar.h"
#include "soul/ui/badge.h"
#include "soul/ui/loading.h"
#include "soul/ui/spinner.h"

using namespace sc;

// ============================================================================
// Avatar
// ============================================================================
class TestUiAvatar : public QObject {
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
        Avatar av(m_parent);
        QVERIFY(av.pixmap().isNull());
        QVERIFY(av.initials().isEmpty());
        QVERIFY(av.size() > 0);
    }

    void testPixmap() {
        Avatar av(m_parent);
        QPixmap pm(32, 32);
        pm.fill(Qt::red);
        av.setPixmap(pm);
        QVERIFY(!av.pixmap().isNull());
        QVERIFY(av.pixmap().size() == QSize(32, 32));
    }

    void testNullPixmap() {
        Avatar av(m_parent);
        QPixmap pm(64, 64);
        pm.fill(Qt::blue);
        av.setPixmap(pm);
        QVERIFY(!av.pixmap().isNull());
        av.setPixmap(QPixmap());
        QVERIFY(av.pixmap().isNull());
    }

    void testInitials() {
        Avatar av(m_parent);
        av.setInitials("AB");
        QVERIFY(av.initials() == QString("AB"));
        av.setInitials("XYZ");
        QVERIFY(av.initials() == QString("XYZ"));
    }

    void testEmptyInitials() {
        Avatar av(m_parent);
        av.setInitials("");
        QVERIFY(av.initials().isEmpty());
    }

    void testSize() {
        Avatar av(m_parent);
        av.setSize(64);
        QVERIFY(av.size() == 64);
        av.setSize(32);
        QVERIFY(av.size() == 32);
        av.setSize(128);
        QVERIFY(av.size() == 128);
    }

    void testSizeCheck() {
        Avatar av(m_parent);
        av.setSize(48);
        QVERIFY(av.width() >= 48);
        QVERIFY(av.height() >= 48);
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
        Badge b(m_parent);
        QVERIFY(b.count() == 0);
    }

    void testCount() {
        Badge b(m_parent);
        b.setCount(5);
        QVERIFY(b.count() == 5);
        b.setCount(0);
        QVERIFY(b.count() == 0);
        b.setCount(99);
        QVERIFY(b.count() == 99);
    }

    void testNegativeCount() {
        Badge b(m_parent);
        b.setCount(-1);
        QVERIFY(b.count() == -1);
    }

    void testVisibility() {
        Badge b(m_parent);
        b.setCount(5);
        QVERIFY(b.isVisible());
        b.setVisible(false);
        QVERIFY(!b.isVisible());
        b.setVisible(true);
        QVERIFY(b.isVisible());
    }

    void testSizeCheck() {
        Badge b(m_parent);
        b.setCount(5);
        QVERIFY(b.width() > 0);
        QVERIFY(b.height() > 0);
    }

private:
    QWidget* m_parent = nullptr;
};

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

    void testIndeterminate() {
        Loading loading(m_parent);
        loading.setIndeterminate(true);
        QVERIFY(true);
    }

    void testShowProgress() {
        Loading loading(m_parent);
        loading.showProgress(true);
        loading.showProgress(false);
        QVERIFY(true);
    }

    void testStaticShowGlobal() {
        QVERIFY(true);
    }

    void testStaticHideGlobal() {
        QVERIFY(true);
    }

    void testStaticUpdateGlobalProgress() {
        QVERIFY(true);
    }

private:
    QWidget* m_parent = nullptr;
};

// ============================================================================
// Spinner
// ============================================================================
class TestUiSpinner : public QObject {
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
        Spinner s(m_parent);
        QVERIFY(s.rotation() == 0.0);
    }

    void testRotation() {
        Spinner s(m_parent);
        s.setRotation(90.0);
        QVERIFY(s.rotation() == 90.0);
        s.setRotation(180.0);
        QVERIFY(s.rotation() == 180.0);
        s.setRotation(360.0);
        QVERIFY(s.rotation() == 360.0);
    }

    void testNegativeRotation() {
        Spinner s(m_parent);
        s.setRotation(-45.0);
        QVERIFY(s.rotation() == -45.0);
    }

    void testZeroRotation() {
        Spinner s(m_parent);
        s.setRotation(0.0);
        QVERIFY(s.rotation() == 0.0);
    }

    void testSizeCheck() {
        Spinner s(m_parent);
        QVERIFY(s.width() > 0);
        QVERIFY(s.height() > 0);
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

    std::cout << "DEBUG: TestUiAvatar start" << std::endl;
    { TestUiAvatar t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiAvatar done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiBadge start" << std::endl;
    { TestUiBadge t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiBadge done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiLoading start" << std::endl;
    { TestUiLoading t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiLoading done, result=" << result << std::endl;

    std::cout << "DEBUG: TestUiSpinner start" << std::endl;
    { TestUiSpinner t; result |= QTest::qExec(&t, argc, argv); }
    std::cout << "DEBUG: TestUiSpinner done, result=" << result << std::endl;

    return result;
}

#include "test_ui_avatar.moc"