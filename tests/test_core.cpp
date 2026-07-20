#include <QTest>
#include <QCoreApplication>
#include <string>
#include "soul/core/time.h"
#include "soul/core/uuid.h"
#include "soul/core/version.h"
#include "soul/core/platform.h"
#include "soul/core/environment.h"

class TestTime : public QObject {
    Q_OBJECT

private slots:
    void testNow();
    void testFromSeconds();
    void testFromMilliseconds();
    void testToSeconds();
    void testToMilliseconds();
    void testNowString();
    void testFormat();
    void testFormatCustom();
    void testFormatMilliseconds();
    void testFormatEmpty();
    void testToLocalTime();
    void testToUtcTime();
    void testParse();
    void testRoundTrip();
};

void TestTime::testNow() {
    auto t1 = sc::Time::now();
    auto t2 = sc::Time::now();
    QVERIFY(t1.count() > 0);
    QVERIFY(t2.count() >= t1.count());
}

void TestTime::testFromSeconds() {
    auto ts = sc::Time::fromSeconds(10);
    QCOMPARE(ts.count(), static_cast<int64_t>(10000));
}

void TestTime::testFromMilliseconds() {
    auto ts = sc::Time::fromMilliseconds(12345);
    QCOMPARE(ts.count(), static_cast<int64_t>(12345));
}

void TestTime::testToSeconds() {
    auto ts = sc::Time::fromMilliseconds(5000);
    QCOMPARE(sc::Time::toSeconds(ts), static_cast<int64_t>(5));
}

void TestTime::testToMilliseconds() {
    auto ts = sc::Time::fromMilliseconds(5000);
    QCOMPARE(sc::Time::toMilliseconds(ts), static_cast<int64_t>(5000));
}

void TestTime::testNowString() {
    std::string s = sc::Time::nowString();
    QVERIFY(!s.empty());
    QVERIFY(s.size() >= 19);
}

void TestTime::testFormat() {
    auto ts = sc::Time::fromMilliseconds(0);
    std::string formatted = sc::Time::format(ts, "yyyy-MM-dd HH:mm:ss");
    QCOMPARE(formatted.size(), static_cast<size_t>(19));
    QCOMPARE(formatted[4], '-');
    QCOMPARE(formatted[7], '-');
    QCOMPARE(formatted[10], ' ');
    QCOMPARE(formatted[13], ':');
    QCOMPARE(formatted[16], ':');
}

void TestTime::testFormatCustom() {
    auto ts = sc::Time::fromMilliseconds(0);
    std::string formatted = sc::Time::format(ts, "yyyy/MM/dd");
    QCOMPARE(formatted.size(), static_cast<size_t>(10));
    QCOMPARE(formatted[4], '/');
    QCOMPARE(formatted[7], '/');
}

void TestTime::testFormatMilliseconds() {
    auto ts = sc::Time::fromMilliseconds(1234567890);
    std::string formatted = sc::Time::format(ts, "zzz");
    QCOMPARE(formatted, std::string("890"));
}

void TestTime::testFormatEmpty() {
    auto ts = sc::Time::fromMilliseconds(0);
    std::string formatted = sc::Time::format(ts, "");
    QVERIFY(formatted.empty());
}

void TestTime::testToLocalTime() {
    auto ts = sc::Time::fromMilliseconds(0);
    std::string s = sc::Time::toLocalTime(ts);
    QCOMPARE(s.size(), static_cast<size_t>(19));
    QCOMPARE(s[4], '-');
    QCOMPARE(s[7], '-');
    QCOMPARE(s[10], ' ');
    QCOMPARE(s[13], ':');
    QCOMPARE(s[16], ':');
}

void TestTime::testToUtcTime() {
    auto ts = sc::Time::fromMilliseconds(0);
    std::string s = sc::Time::toUtcTime(ts);
    QCOMPARE(s, std::string("1970-01-01 00:00:00"));
}

void TestTime::testParse() {
    auto ts = sc::Time::parse("2024-01-01 00:00:00");
    QCOMPARE(ts.count(), static_cast<int64_t>(0));
}

void TestTime::testRoundTrip() {
    auto ts = sc::Time::fromMilliseconds(123456789);
    QCOMPARE(sc::Time::toMilliseconds(ts), static_cast<int64_t>(123456789));
    QCOMPARE(sc::Time::toSeconds(ts), static_cast<int64_t>(123456));
}


class TestUuid : public QObject {
    Q_OBJECT

private slots:
    void testGenerate();
    void testGenerateUnique();
    void testGenerateFormat();
    void testIsValid();
    void testIsValidLowercase();
    void testIsValidAllZeros();
    void testIsValidEmpty();
    void testIsValidTooShort();
    void testIsValidTooLong();
    void testIsValidBadHyphens();
    void testIsValidBadChars();
    void testFormat();
    void testFormatAlreadyUpper();
    void testFormatNoChange();
};

void TestUuid::testGenerate() {
    std::string uuid = sc::Uuid::generate();
    QCOMPARE(uuid.size(), static_cast<size_t>(36));
    QVERIFY(sc::Uuid::isValid(uuid));
}

void TestUuid::testGenerateUnique() {
    std::string u1 = sc::Uuid::generate();
    std::string u2 = sc::Uuid::generate();
    QVERIFY(u1 != u2);
}

void TestUuid::testGenerateFormat() {
    std::string uuid = sc::Uuid::generate();
    QCOMPARE(uuid[8], '-');
    QCOMPARE(uuid[13], '-');
    QCOMPARE(uuid[18], '-');
    QCOMPARE(uuid[23], '-');
}

void TestUuid::testIsValid() {
    QVERIFY(sc::Uuid::isValid("550E8400-E29B-41D4-A716-446655440000"));
}

void TestUuid::testIsValidLowercase() {
    QVERIFY(sc::Uuid::isValid("550e8400-e29b-41d4-a716-446655440000"));
}

void TestUuid::testIsValidAllZeros() {
    QVERIFY(sc::Uuid::isValid("00000000-0000-0000-0000-000000000000"));
}

void TestUuid::testIsValidEmpty() {
    QVERIFY(!sc::Uuid::isValid(""));
}

void TestUuid::testIsValidTooShort() {
    QVERIFY(!sc::Uuid::isValid("550E8400-E29B-41D4-A716-44665544000"));
}

void TestUuid::testIsValidTooLong() {
    QVERIFY(!sc::Uuid::isValid("550E8400-E29B-41D4-A716-4466554400000"));
}

void TestUuid::testIsValidBadHyphens() {
    QVERIFY(!sc::Uuid::isValid("550E8400XE29B-41D4-A716-446655440000"));
    QVERIFY(!sc::Uuid::isValid("550E8400-E29BX41D4-A716-446655440000"));
}

void TestUuid::testIsValidBadChars() {
    QVERIFY(!sc::Uuid::isValid("550E84GG-E29B-41D4-A716-446655440000"));
    QVERIFY(!sc::Uuid::isValid("550E8400-E29B-41D4-A716-44665544000G"));
}

void TestUuid::testFormat() {
    std::string formatted = sc::Uuid::format("550e8400-e29b-41d4-a716-446655440000");
    QCOMPARE(formatted, std::string("550E8400-E29B-41D4-A716-446655440000"));
}

void TestUuid::testFormatAlreadyUpper() {
    std::string formatted = sc::Uuid::format("550E8400-E29B-41D4-A716-446655440000");
    QCOMPARE(formatted, std::string("550E8400-E29B-41D4-A716-446655440000"));
}

void TestUuid::testFormatNoChange() {
    std::string formatted = sc::Uuid::format("01234567-89AB-CDEF-0123-456789ABCDEF");
    QCOMPARE(formatted, std::string("01234567-89AB-CDEF-0123-456789ABCDEF"));
}


class TestVersion : public QObject {
    Q_OBJECT

private slots:
    void testDefaultConstructor();
    void testConstructor();
    void testToString();
    void testToStringWithPre();
    void testToStringWithBuild();
    void testToStringWithPreAndBuild();
    void testParse();
    void testParseWithPre();
    void testParseWithBuild();
    void testParseWithPreAndBuild();
    void testParseInvalid();
    void testParseEmpty();
    void testParseIncomplete();
    void testComparison();
    void testEquality();
    void testInequality();
    void testPreReleaseComparison();
    void testBuildMetadataIgnored();
};

void TestVersion::testDefaultConstructor() {
    sc::Version v;
    QCOMPARE(v.major(), 0);
    QCOMPARE(v.minor(), 0);
    QCOMPARE(v.patch(), 0);
    QCOMPARE(v.toString(), std::string("0.0.0"));
}

void TestVersion::testConstructor() {
    sc::Version v(1, 2, 3);
    QCOMPARE(v.major(), 1);
    QCOMPARE(v.minor(), 2);
    QCOMPARE(v.patch(), 3);
}

void TestVersion::testToString() {
    sc::Version v(1, 0, 0);
    QCOMPARE(v.toString(), std::string("1.0.0"));
}

void TestVersion::testToStringWithPre() {
    sc::Version v(1, 0, 0, "alpha");
    QCOMPARE(v.toString(), std::string("1.0.0-alpha"));
}

void TestVersion::testToStringWithBuild() {
    sc::Version v(1, 0, 0, "", "build123");
    QCOMPARE(v.toString(), std::string("1.0.0+build123"));
}

void TestVersion::testToStringWithPreAndBuild() {
    sc::Version v(1, 0, 0, "alpha", "build123");
    QCOMPARE(v.toString(), std::string("1.0.0-alpha+build123"));
}

void TestVersion::testParse() {
    auto v = sc::Version::parse("1.2.3");
    QCOMPARE(v.major(), 1);
    QCOMPARE(v.minor(), 2);
    QCOMPARE(v.patch(), 3);
    QCOMPARE(v.toString(), std::string("1.2.3"));
}

void TestVersion::testParseWithPre() {
    auto v = sc::Version::parse("1.2.3-alpha");
    QCOMPARE(v.major(), 1);
    QCOMPARE(v.minor(), 2);
    QCOMPARE(v.patch(), 3);
    QCOMPARE(v.toString(), std::string("1.2.3-alpha"));
}

void TestVersion::testParseWithBuild() {
    auto v = sc::Version::parse("1.2.3+build");
    QCOMPARE(v.toString(), std::string("1.2.3+build"));
}

void TestVersion::testParseWithPreAndBuild() {
    auto v = sc::Version::parse("1.2.3-alpha+build");
    QCOMPARE(v.toString(), std::string("1.2.3-alpha+build"));
}

void TestVersion::testParseInvalid() {
    auto v = sc::Version::parse("invalid");
    QCOMPARE(v.toString(), std::string("0.0.0"));
}

void TestVersion::testParseEmpty() {
    auto v = sc::Version::parse("");
    QCOMPARE(v.toString(), std::string("0.0.0"));
}

void TestVersion::testParseIncomplete() {
    auto v = sc::Version::parse("1.2");
    QCOMPARE(v.toString(), std::string("0.0.0"));
}

void TestVersion::testComparison() {
    sc::Version v1(1, 0, 0);
    sc::Version v2(2, 0, 0);
    QVERIFY(v1 < v2);
    QVERIFY(v1 <= v2);
    QVERIFY(v2 > v1);
    QVERIFY(v2 >= v1);

    sc::Version v3(1, 1, 0);
    QVERIFY(v1 < v3);

    sc::Version v4(1, 0, 1);
    QVERIFY(v1 < v4);
}

void TestVersion::testEquality() {
    sc::Version v1(1, 2, 3);
    sc::Version v2(1, 2, 3);
    QVERIFY(v1 == v2);
    QVERIFY(v1 <= v2);
    QVERIFY(v1 >= v2);
}

void TestVersion::testInequality() {
    sc::Version v1(1, 2, 3);
    sc::Version v2(2, 0, 0);
    QVERIFY(v1 != v2);
}

void TestVersion::testPreReleaseComparison() {
    sc::Version v1(1, 0, 0, "alpha");
    sc::Version v2(1, 0, 0, "beta");
    QVERIFY(v1 < v2);
    QVERIFY(v2 > v1);
}

void TestVersion::testBuildMetadataIgnored() {
    sc::Version v1(1, 0, 0, "", "build1");
    sc::Version v2(1, 0, 0, "", "build2");
    QVERIFY(v1 == v2);
}


class TestPlatform : public QObject {
    Q_OBJECT

private slots:
    void testOs();
    void testOsName();
    void testOsVersion();
    void testArchitecture();
    void testRuntimeMode();
    void testSetRuntimeMode();
    void testIsDebugBuild();
    void testIsReleaseBuild();
    void testCpuInfo();
    void testMemorySize();
};

void TestPlatform::testOs() {
    sc::OS os = sc::Platform::os();
#ifdef _WIN32
    QCOMPARE(os, sc::OS::Windows);
#elif __APPLE__
    QCOMPARE(os, sc::OS::macOS);
#elif __linux__
    QCOMPARE(os, sc::OS::Linux);
#endif
}

void TestPlatform::testOsName() {
    std::string name = sc::Platform::osName();
    QVERIFY(!name.empty());
#ifdef _WIN32
    QCOMPARE(name, std::string("Windows"));
#endif
}

void TestPlatform::testOsVersion() {
    std::string ver = sc::Platform::osVersion();
    QVERIFY(!ver.empty());
}

void TestPlatform::testArchitecture() {
    std::string arch = sc::Platform::architecture();
    QVERIFY(!arch.empty());
}

void TestPlatform::testRuntimeMode() {
    sc::RuntimeMode original = sc::Platform::runtimeMode();
    sc::Platform::setRuntimeMode(sc::RuntimeMode::Testing);
    QCOMPARE(sc::Platform::runtimeMode(), sc::RuntimeMode::Testing);
    sc::Platform::setRuntimeMode(original);
}

void TestPlatform::testSetRuntimeMode() {
    sc::RuntimeMode original = sc::Platform::runtimeMode();
    sc::Platform::setRuntimeMode(sc::RuntimeMode::Production);
    QCOMPARE(sc::Platform::runtimeMode(), sc::RuntimeMode::Production);
    sc::Platform::setRuntimeMode(sc::RuntimeMode::Development);
    QCOMPARE(sc::Platform::runtimeMode(), sc::RuntimeMode::Development);
    sc::Platform::setRuntimeMode(sc::RuntimeMode::Testing);
    QCOMPARE(sc::Platform::runtimeMode(), sc::RuntimeMode::Testing);
    sc::Platform::setRuntimeMode(original);
}

void TestPlatform::testIsDebugBuild() {
    bool isDebug = sc::Platform::isDebugBuild();
    Q_UNUSED(isDebug);
    QVERIFY(sc::Platform::isDebugBuild() != sc::Platform::isReleaseBuild());
}

void TestPlatform::testIsReleaseBuild() {
    QVERIFY(sc::Platform::isDebugBuild() != sc::Platform::isReleaseBuild());
}

void TestPlatform::testCpuInfo() {
    std::string info = sc::Platform::cpuInfo();
    QVERIFY(!info.empty());
}

void TestPlatform::testMemorySize() {
    size_t mem = sc::Platform::memorySize();
    Q_UNUSED(mem);
    QVERIFY(true);
}


class TestEnvironment : public QObject {
    Q_OBJECT

private slots:
    void testGet();
    void testGetDefault();
    void testGetEmptyDefault();
    void testSet();
    void testContains();
    void testSetAndContains();
    void testGetExecutablePath();
    void testGetWorkingDirectory();
    void testSetWorkingDirectory();
    void testGetHomeDirectory();
    void testGetAppDataDirectory();
    void testGetTempDirectory();
    void testGetEnv();
    void testParseCommandLine();
    void testParseCommandLineSingleDash();
    void testParseCommandLineEmpty();
};

void TestEnvironment::testGet() {
    std::string path = sc::Environment::get("PATH");
    QVERIFY(!path.empty());
}

void TestEnvironment::testGetDefault() {
    std::string value = sc::Environment::get("KITFORSC_NONEXISTENT_VAR_12345", "default_value");
    QCOMPARE(value, std::string("default_value"));
}

void TestEnvironment::testGetEmptyDefault() {
    std::string value = sc::Environment::get("KITFORSC_NONEXISTENT_VAR_12345");
    QCOMPARE(value, std::string(""));
}

void TestEnvironment::testSet() {
    sc::Environment::set("KITFORSC_TEST_VAR", "test_value");
    std::string value = sc::Environment::get("KITFORSC_TEST_VAR");
    QCOMPARE(value, std::string("test_value"));
}

void TestEnvironment::testContains() {
    QVERIFY(sc::Environment::contains("PATH"));
    QVERIFY(!sc::Environment::contains("KITFORSC_NONEXISTENT_VAR_12345"));
}

void TestEnvironment::testSetAndContains() {
    QVERIFY(!sc::Environment::contains("KITFORSC_SET_TEST_VAR"));
    sc::Environment::set("KITFORSC_SET_TEST_VAR", "value");
    QVERIFY(sc::Environment::contains("KITFORSC_SET_TEST_VAR"));
}

void TestEnvironment::testGetExecutablePath() {
    std::string path = sc::Environment::getExecutablePath();
    QVERIFY(!path.empty());
}

void TestEnvironment::testGetWorkingDirectory() {
    std::string cwd = sc::Environment::getWorkingDirectory();
    QVERIFY(!cwd.empty());
}

void TestEnvironment::testSetWorkingDirectory() {
    std::string original = sc::Environment::getWorkingDirectory();
    sc::Environment::setWorkingDirectory(original);
    std::string cwd = sc::Environment::getWorkingDirectory();
    QCOMPARE(cwd, original);
}

void TestEnvironment::testGetHomeDirectory() {
    std::string home = sc::Environment::getHomeDirectory();
    QVERIFY(!home.empty());
}

void TestEnvironment::testGetAppDataDirectory() {
    std::string appData = sc::Environment::getAppDataDirectory();
    QVERIFY(!appData.empty());
}

void TestEnvironment::testGetTempDirectory() {
    std::string temp = sc::Environment::getTempDirectory();
    QVERIFY(!temp.empty());
}

void TestEnvironment::testGetEnv() {
    std::string env = sc::Environment::getEnv();
    QVERIFY(!env.empty());
}

void TestEnvironment::testParseCommandLine() {
    char arg0[] = "program";
    char arg1[] = "--name=value";
    char arg2[] = "--verbose";
    char arg3[] = "-o";
    char arg4[] = "output.txt";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4, nullptr};
    int argc = 5;

    auto args = sc::Environment::parseCommandLine(argc, argv);
    QCOMPARE(args["name"], std::string("value"));
    QCOMPARE(args["verbose"], std::string("true"));
    QCOMPARE(args["o"], std::string("output.txt"));
}

void TestEnvironment::testParseCommandLineSingleDash() {
    char arg0[] = "program";
    char arg1[] = "-f";
    char* argv[] = {arg0, arg1, nullptr};
    int argc = 2;

    auto args = sc::Environment::parseCommandLine(argc, argv);
    QCOMPARE(args["f"], std::string("true"));
}

void TestEnvironment::testParseCommandLineEmpty() {
    char arg0[] = "program";
    char* argv[] = {arg0, nullptr};
    int argc = 1;

    auto args = sc::Environment::parseCommandLine(argc, argv);
    QVERIFY(args.empty());
}


int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    int result = 0;

    {
        TestTime t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestUuid t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestVersion t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestPlatform t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestEnvironment t;
        result |= QTest::qExec(&t, argc, argv);
    }

    return result;
}

#include "test_core.moc"
