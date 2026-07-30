#include <QTest>
#include <QApplication>
#include <QWidget>
#include <iostream>

#include "soul/ui/glass_widget.h"

class TestMinimal : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        m_parent = new QWidget();
        m_parent->resize(400, 300);
        m_parent->show();
    }

    void cleanupTestCase() {
        delete m_parent;
    }

    void testCreateGlassWidget() {
        ui::GlassWidget gw(m_parent);
        QVERIFY(gw.blurRadius() >= 0);
    }

    void testSetBlurRadius() {
        ui::GlassWidget gw(m_parent);
        gw.setBlurRadius(20);
        QVERIFY(gw.blurRadius() == 20);
    }

private:
    QWidget* m_parent = nullptr;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    TestMinimal t;
    return QTest::qExec(&t, argc, argv);
}

#include "test_glass_minimal.moc"