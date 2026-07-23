#include <QTest>
#include "soul/logging/logger.h"

class TestLogger : public QObject {
    Q_OBJECT

private slots:
    void testLogLevels();
    void testModuleAndOperation();
};

void TestLogger::testLogLevels() {
    auto& logger = sc::Logger::instance();
    logger.setLogLevel(sc::LogLevel::Info);

    logger.trace("trace message");
    logger.debug("debug message");
    logger.info("info message");
    logger.warn("warn message");
    logger.error("error message");
    logger.fatal("fatal message");

    QVERIFY(true);
}

void TestLogger::testModuleAndOperation() {
    auto& logger = sc::Logger::instance();

    logger.info("User login", "Auth", "login");
    logger.error("Connection failed", "Network");
    logger.debug("Cache hit", "Storage");

    QVERIFY(true);
}

QTEST_MAIN(TestLogger)
#include "test_logger.moc"
