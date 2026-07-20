#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include "soul/utils/string/string_utils.h"
#include "soul/utils/json/json_utils.h"
#include "soul/utils/datetime/datetime_utils.h"
#include "soul/utils/crypto/crypto_utils.h"
#include "soul/utils/file/file_utils.h"
#include "soul/utils/xml/xml_utils.h"
#include "soul/utils/compress/compress_utils.h"

class TestStringUtils : public QObject {
    Q_OBJECT
private slots:
    void testTrim();
    void testTrimLeft();
    void testTrimRight();
    void testToLower();
    void testToUpper();
    void testStartsWith();
    void testEndsWith();
    void testContains();
    void testSplit();
    void testJoin();
    void testReplace();
    void testIsEmpty();
    void testIsBlank();
    void testCount();
    void testSubstring();
};

void TestStringUtils::testTrim() {
    QCOMPARE(sc::utils::string::trim("  hello  "), QString("hello"));
    QCOMPARE(sc::utils::string::trim(""), QString(""));
    QCOMPARE(sc::utils::string::trim("   "), QString(""));
    QCOMPARE(sc::utils::string::trim("hello"), QString("hello"));
}

void TestStringUtils::testTrimLeft() {
    QCOMPARE(sc::utils::string::trimLeft("  hello  "), QString("hello  "));
    QCOMPARE(sc::utils::string::trimLeft(""), QString(""));
    QCOMPARE(sc::utils::string::trimLeft("   "), QString(""));
}

void TestStringUtils::testTrimRight() {
    QCOMPARE(sc::utils::string::trimRight("  hello  "), QString("  hello"));
    QCOMPARE(sc::utils::string::trimRight(""), QString(""));
    QCOMPARE(sc::utils::string::trimRight("   "), QString(""));
}

void TestStringUtils::testToLower() {
    QCOMPARE(sc::utils::string::toLower("HELLO"), QString("hello"));
    QCOMPARE(sc::utils::string::toLower("Hello World"), QString("hello world"));
    QCOMPARE(sc::utils::string::toLower(""), QString(""));
}

void TestStringUtils::testToUpper() {
    QCOMPARE(sc::utils::string::toUpper("hello"), QString("HELLO"));
    QCOMPARE(sc::utils::string::toUpper("Hello World"), QString("HELLO WORLD"));
    QCOMPARE(sc::utils::string::toUpper(""), QString(""));
}

void TestStringUtils::testStartsWith() {
    QVERIFY(sc::utils::string::startsWith("hello world", "hello"));
    QVERIFY(!sc::utils::string::startsWith("hello world", "world"));
    QVERIFY(sc::utils::string::startsWith("", ""));
    QVERIFY(!sc::utils::string::startsWith("hello", ""));
}

void TestStringUtils::testEndsWith() {
    QVERIFY(sc::utils::string::endsWith("hello world", "world"));
    QVERIFY(!sc::utils::string::endsWith("hello world", "hello"));
    QVERIFY(sc::utils::string::endsWith("", ""));
}

void TestStringUtils::testContains() {
    QVERIFY(sc::utils::string::contains("hello world", "world"));
    QVERIFY(!sc::utils::string::contains("hello world", "foo"));
    QVERIFY(!sc::utils::string::contains("", "foo"));
}

void TestStringUtils::testSplit() {
    auto parts = sc::utils::string::split("a,b,c", ",");
    QCOMPARE(static_cast<int>(parts.size()), 3);
    QCOMPARE(parts[0], QString("a"));
    QCOMPARE(parts[1], QString("b"));
    QCOMPARE(parts[2], QString("c"));

    parts = sc::utils::string::split("", ",");
    QCOMPARE(static_cast<int>(parts.size()), 1);
    QCOMPARE(parts[0], QString(""));

    parts = sc::utils::string::split("hello", ",");
    QCOMPARE(static_cast<int>(parts.size()), 1);
    QCOMPARE(parts[0], QString("hello"));
}

void TestStringUtils::testJoin() {
    std::vector<QString> parts = {"a", "b", "c"};
    QCOMPARE(sc::utils::string::join(parts, ","), QString("a,b,c"));

    parts.clear();
    QCOMPARE(sc::utils::string::join(parts, ","), QString(""));
}

void TestStringUtils::testReplace() {
    QCOMPARE(sc::utils::string::replace("hello world", "world", "foo"), QString("hello foo"));
    QCOMPARE(sc::utils::string::replace("hello world", "nonexistent", "foo"), QString("hello world"));
    QCOMPARE(sc::utils::string::replace("", "a", "b"), QString(""));
}

void TestStringUtils::testIsEmpty() {
    QVERIFY(sc::utils::string::isEmpty(""));
    QVERIFY(!sc::utils::string::isEmpty("hello"));
    QVERIFY(!sc::utils::string::isEmpty("   "));
}

void TestStringUtils::testIsBlank() {
    QVERIFY(sc::utils::string::isBlank(""));
    QVERIFY(sc::utils::string::isBlank("   "));
    QVERIFY(!sc::utils::string::isBlank("hello"));
    QVERIFY(!sc::utils::string::isBlank(" hello "));
}

void TestStringUtils::testCount() {
    QCOMPARE(sc::utils::string::count("hello world", "l"), 3);
    QCOMPARE(sc::utils::string::count("hello world", "hello"), 1);
    QCOMPARE(sc::utils::string::count("hello world", "nonexistent"), 0);
    QCOMPARE(sc::utils::string::count("", "a"), 0);
}

void TestStringUtils::testSubstring() {
    QCOMPARE(sc::utils::string::substring("hello", 0, 2), QString("he"));
    QCOMPARE(sc::utils::string::substring("hello", 2), QString("llo"));
    QCOMPARE(sc::utils::string::substring("hello", 0, 10), QString("hello"));
    QCOMPARE(sc::utils::string::substring("hello", 5), QString(""));
}

class TestJsonUtils : public QObject {
    Q_OBJECT
private slots:
    void testGetString();
    void testGetInt();
    void testGetDouble();
    void testGetBool();
    void testGetArray();
    void testGetObject();
    void testContains();
    void testToPrettyString();
    void testToCompactString();
};

void TestJsonUtils::testGetString() {
    QJsonObject obj;
    obj["name"] = "test";
    obj["empty"] = "";

    QCOMPARE(sc::utils::json::getString(obj, "name"), QString("test"));
    QCOMPARE(sc::utils::json::getString(obj, "missing", "default"), QString("default"));
    QCOMPARE(sc::utils::json::getString(obj, "empty"), QString(""));
}

void TestJsonUtils::testGetInt() {
    QJsonObject obj;
    obj["count"] = 42;

    QCOMPARE(sc::utils::json::getInt(obj, "count"), 42);
    QCOMPARE(sc::utils::json::getInt(obj, "missing", 10), 10);
}

void TestJsonUtils::testGetDouble() {
    QJsonObject obj;
    obj["price"] = 3.14;

    QCOMPARE(sc::utils::json::getDouble(obj, "price"), 3.14);
    QCOMPARE(sc::utils::json::getDouble(obj, "missing", 0.5), 0.5);
}

void TestJsonUtils::testGetBool() {
    QJsonObject obj;
    obj["enabled"] = true;
    obj["disabled"] = false;

    QVERIFY(sc::utils::json::getBool(obj, "enabled"));
    QVERIFY(!sc::utils::json::getBool(obj, "disabled"));
    QVERIFY(!sc::utils::json::getBool(obj, "missing"));
    QVERIFY(sc::utils::json::getBool(obj, "missing", true));
}

void TestJsonUtils::testGetArray() {
    QJsonObject obj;
    QJsonArray array;
    array.append("a");
    array.append("b");
    obj["items"] = array;

    QJsonArray result = sc::utils::json::getArray(obj, "items");
    QCOMPARE(result.size(), 2);
    QCOMPARE(result[0].toString(), QString("a"));
}

void TestJsonUtils::testGetObject() {
    QJsonObject obj;
    QJsonObject nested;
    nested["value"] = 100;
    obj["nested"] = nested;

    QJsonObject result = sc::utils::json::getObject(obj, "nested");
    QVERIFY(result.contains("value"));
    QCOMPARE(result["value"].toInt(), 100);
}

void TestJsonUtils::testContains() {
    QJsonObject obj;
    obj["key"] = "value";

    QVERIFY(sc::utils::json::contains(obj, "key"));
    QVERIFY(!sc::utils::json::contains(obj, "missing"));
}

void TestJsonUtils::testToPrettyString() {
    QJsonObject obj;
    obj["name"] = "test";
    QJsonDocument doc(obj);

    QString pretty = sc::utils::json::toPrettyString(doc);
    QVERIFY(pretty.contains("\"name\""));
    QVERIFY(pretty.contains("\"test\""));
}

void TestJsonUtils::testToCompactString() {
    QJsonObject obj;
    obj["name"] = "test";
    QJsonDocument doc(obj);

    QString compact = sc::utils::json::toCompactString(doc);
    QVERIFY(compact.contains("\"name\":\"test\""));
}

class TestDateTimeUtils : public QObject {
    Q_OBJECT
private slots:
    void testFormat();
    void testFormatDate();
    void testFormatTime();
    void testToISO8601();
    void testFromISO8601();
    void testToTimestamp();
    void testFromTimestamp();
    void testNow();
    void testToday();
    void testIsLeapYear();
    void testAddDays();
    void testDiffDays();
};

void TestDateTimeUtils::testFormat() {
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30, 45));
    QCOMPARE(sc::DateTimeUtils::format(dt), QString("2024-01-15 10:30:45"));
    QCOMPARE(sc::DateTimeUtils::format(dt, "yyyy-MM-dd"), QString("2024-01-15"));
}

void TestDateTimeUtils::testFormatDate() {
    QDate date(2024, 6, 15);
    QCOMPARE(sc::DateTimeUtils::formatDate(date), QString("2024-06-15"));
    QCOMPARE(sc::DateTimeUtils::formatDate(date, "MM/dd/yyyy"), QString("06/15/2024"));
}

void TestDateTimeUtils::testFormatTime() {
    QTime time(14, 30, 0);
    QCOMPARE(sc::DateTimeUtils::formatTime(time), QString("14:30:00"));
    QCOMPARE(sc::DateTimeUtils::formatTime(time, "hh:mm AP"), QString("02:30 PM"));
}

void TestDateTimeUtils::testToISO8601() {
    QDateTime dt(QDate(2024, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QString iso = sc::DateTimeUtils::toISO8601(dt);
    QVERIFY(iso.startsWith("2024-01-01"));
}

void TestDateTimeUtils::testFromISO8601() {
    QString iso = "2024-01-15T10:30:45Z";
    QDateTime dt = sc::DateTimeUtils::fromISO8601(iso);
    QCOMPARE(dt.date(), QDate(2024, 1, 15));
}

void TestDateTimeUtils::testToTimestamp() {
    QDateTime dt(QDate(1970, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QCOMPARE(sc::DateTimeUtils::toTimestamp(dt), static_cast<qint64>(0));
}

void TestDateTimeUtils::testFromTimestamp() {
    QDateTime dt = sc::DateTimeUtils::fromTimestamp(0);
    QCOMPARE(dt.date(), QDate(1970, 1, 1));
}

void TestDateTimeUtils::testNow() {
    QDateTime now = sc::DateTimeUtils::now();
    QVERIFY(now.isValid());
}

void TestDateTimeUtils::testToday() {
    QDate today = sc::DateTimeUtils::today();
    QVERIFY(today.isValid());
    QCOMPARE(today.year(), QDate::currentDate().year());
}

void TestDateTimeUtils::testIsLeapYear() {
    QVERIFY(sc::DateTimeUtils::isLeapYear(2024));
    QVERIFY(!sc::DateTimeUtils::isLeapYear(2023));
    QVERIFY(sc::DateTimeUtils::isLeapYear(2000));
    QVERIFY(!sc::DateTimeUtils::isLeapYear(1900));
}

void TestDateTimeUtils::testAddDays() {
    QDateTime dt(QDate(2024, 1, 1), QTime(0, 0, 0));
    QDateTime result = sc::DateTimeUtils::addDays(dt, 1);
    QCOMPARE(result.date(), QDate(2024, 1, 2));
}

void TestDateTimeUtils::testDiffDays() {
    QDateTime start(QDate(2024, 1, 1), QTime(0, 0, 0));
    QDateTime end(QDate(2024, 1, 10), QTime(0, 0, 0));
    QCOMPARE(sc::DateTimeUtils::diffDays(start, end), static_cast<qint64>(9));
}

class TestCryptoUtils : public QObject {
    Q_OBJECT
private slots:
    void testMd5();
    void testMd5Hex();
    void testSha256();
    void testSha256Hex();
    void testBase64Encode();
    void testBase64Decode();
    void testUrlEncode();
    void testUrlDecode();
    void testGenerateRandomBytes();
    void testGenerateRandomString();
};

void TestCryptoUtils::testMd5() {
    QByteArray result = sc::utils::crypto::md5("hello");
    QVERIFY(!result.isEmpty());
    QCOMPARE(result.size(), 16);
}

void TestCryptoUtils::testMd5Hex() {
    QString result = sc::utils::crypto::md5Hex("hello");
    QVERIFY(!result.isEmpty());
    QCOMPARE(result.length(), 32);
}

void TestCryptoUtils::testSha256() {
    QByteArray result = sc::utils::crypto::sha256("hello");
    QVERIFY(!result.isEmpty());
    QCOMPARE(result.size(), 32);
}

void TestCryptoUtils::testSha256Hex() {
    QString result = sc::utils::crypto::sha256Hex("hello");
    QVERIFY(!result.isEmpty());
    QCOMPARE(result.length(), 64);
}

void TestCryptoUtils::testBase64Encode() {
    QString encoded = sc::utils::crypto::base64Encode("hello");
    QVERIFY(!encoded.isEmpty());
}

void TestCryptoUtils::testBase64Decode() {
    QString encoded = sc::utils::crypto::base64Encode("hello");
    QByteArray decoded = sc::utils::crypto::base64Decode(encoded);
    QCOMPARE(decoded, QByteArray("hello"));
}

void TestCryptoUtils::testUrlEncode() {
    QString result = sc::utils::crypto::urlEncode("hello world");
    QVERIFY(result.contains("%20"));
}

void TestCryptoUtils::testUrlDecode() {
    QString encoded = sc::utils::crypto::urlEncode("hello world");
    QString decoded = sc::utils::crypto::urlDecode(encoded);
    QCOMPARE(decoded, QString("hello world"));
}

void TestCryptoUtils::testGenerateRandomBytes() {
    QByteArray bytes = sc::utils::crypto::generateRandomBytes(16);
    QCOMPARE(bytes.size(), 16);
    QVERIFY(!bytes.isEmpty());
}

void TestCryptoUtils::testGenerateRandomString() {
    QString str = sc::utils::crypto::generateRandomString(16);
    QCOMPARE(str.length(), 16);
    QVERIFY(!str.isEmpty());
}

class TestFileUtils : public QObject {
    Q_OBJECT
private slots:
    void testExists();
    void testIsFile();
    void testIsDirectory();
    void testCreateDirectory();
    void testReadWriteFile();
    void testCopy();
    void testRemove();
    void testFileName();
    void testBaseName();
    void testSuffix();
    void testDirectory();
    void testJoinPath();
};

void TestFileUtils::testExists() {
    QTemporaryFile tempFile;
    tempFile.open();
    QString path = tempFile.fileName();
    tempFile.close();

    QVERIFY(sc::utils::file::exists(path));
    QVERIFY(!sc::utils::file::exists("/nonexistent/path"));
}

void TestFileUtils::testIsFile() {
    QTemporaryFile tempFile;
    tempFile.open();
    QString path = tempFile.fileName();
    tempFile.close();

    QVERIFY(sc::utils::file::isFile(path));
    QVERIFY(!sc::utils::file::isFile(QTemporaryDir().path()));
}

void TestFileUtils::testIsDirectory() {
    QTemporaryDir tempDir;
    QVERIFY(sc::utils::file::isDirectory(tempDir.path()));
    QTemporaryFile tempFile;
    tempFile.open();
    QVERIFY(!sc::utils::file::isDirectory(tempFile.fileName()));
}

void TestFileUtils::testCreateDirectory() {
    QTemporaryDir tempDir;
    QString newDir = sc::utils::file::joinPath(tempDir.path(), "newdir");
    QVERIFY(sc::utils::file::createDirectory(newDir));
    QVERIFY(sc::utils::file::exists(newDir));
}

void TestFileUtils::testReadWriteFile() {
    QTemporaryFile tempFile;
    tempFile.open();
    QString path = tempFile.fileName();
    tempFile.close();

    QByteArray content = "test content";
    QVERIFY(sc::utils::file::writeFile(path, content));
    QByteArray readContent = sc::utils::file::readFile(path);
    QCOMPARE(readContent, content);
}

void TestFileUtils::testCopy() {
    QTemporaryDir tempDir;
    QString source = sc::utils::file::joinPath(tempDir.path(), "source.txt");
    QString dest = sc::utils::file::joinPath(tempDir.path(), "dest.txt");

    sc::utils::file::writeFile(source, "test");
    QVERIFY(sc::utils::file::copy(source, dest));
    QVERIFY(sc::utils::file::exists(dest));
    QCOMPARE(sc::utils::file::readFile(dest), QByteArray("test"));
}

void TestFileUtils::testRemove() {
    QTemporaryFile tempFile;
    tempFile.open();
    QString path = tempFile.fileName();
    tempFile.close();

    QVERIFY(sc::utils::file::exists(path));
    QVERIFY(sc::utils::file::remove(path));
    QVERIFY(!sc::utils::file::exists(path));
}

void TestFileUtils::testFileName() {
    QCOMPARE(sc::utils::file::fileName("/path/to/file.txt"), QString("file.txt"));
    QCOMPARE(sc::utils::file::fileName("/path/to/"), QString(""));
}

void TestFileUtils::testBaseName() {
    QCOMPARE(sc::utils::file::baseName("/path/to/file.txt"), QString("file"));
    QCOMPARE(sc::utils::file::baseName("/path/to/file"), QString("file"));
}

void TestFileUtils::testSuffix() {
    QCOMPARE(sc::utils::file::suffix("/path/to/file.txt"), QString("txt"));
    QCOMPARE(sc::utils::file::suffix("/path/to/file"), QString(""));
}

void TestFileUtils::testDirectory() {
    QCOMPARE(sc::utils::file::directory("/path/to/file.txt"), QString("/path/to"));
    QCOMPARE(sc::utils::file::directory("/file.txt"), QString(""));
}

void TestFileUtils::testJoinPath() {
    QCOMPARE(sc::utils::file::joinPath("/path", "to"), QString("/path/to"));
    QCOMPARE(sc::utils::file::joinPath("/path/", "/to"), QString("/path/to"));
}

class TestXmlUtils : public QObject {
    Q_OBJECT
private slots:
    void testParse();
    void testSerialize();
    void testCreateElement();
    void testGetText();
    void testGetAttribute();
    void testFindElement();
};

void TestXmlUtils::testParse() {
    QString xml = "<root><child>text</child></root>";
    QDomDocument doc = sc::XmlUtils::parse(xml);
    QVERIFY(!doc.isNull());
    QVERIFY(doc.documentElement().tagName() == "root");
}

void TestXmlUtils::testSerialize() {
    QDomDocument doc;
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);
    QDomElement child = doc.createElement("child");
    child.appendChild(doc.createTextNode("text"));
    root.appendChild(child);

    QString serialized = sc::XmlUtils::serialize(doc);
    QVERIFY(serialized.contains("<root>"));
    QVERIFY(serialized.contains("<child>text</child>"));
}

void TestXmlUtils::testCreateElement() {
    QDomDocument doc;
    QDomElement element = sc::XmlUtils::createElement(doc, "tag", "text");
    QCOMPARE(element.tagName(), QString("tag"));
    QCOMPARE(sc::XmlUtils::getText(element), QString("text"));
}

void TestXmlUtils::testGetText() {
    QDomDocument doc;
    QDomElement element = doc.createElement("tag");
    element.appendChild(doc.createTextNode("hello"));
    QCOMPARE(sc::XmlUtils::getText(element), QString("hello"));
}

void TestXmlUtils::testGetAttribute() {
    QDomDocument doc;
    QDomElement element = doc.createElement("tag");
    element.setAttribute("attr", "value");

    QCOMPARE(sc::XmlUtils::getAttribute(element, "attr"), QString("value"));
    QCOMPARE(sc::XmlUtils::getAttribute(element, "missing", "default"), QString("default"));
}

void TestXmlUtils::testFindElement() {
    QString xml = "<root><child1>text1</child1><child2>text2</child2></root>";
    QDomDocument doc = sc::XmlUtils::parse(xml);
    QDomElement root = doc.documentElement();

    QDomElement child1 = sc::XmlUtils::findElement(root, "child1");
    QVERIFY(!child1.isNull());
    QCOMPARE(sc::XmlUtils::getText(child1), QString("text1"));

    QDomElement missing = sc::XmlUtils::findElement(root, "missing");
    QVERIFY(missing.isNull());
}

class TestCompressUtils : public QObject {
    Q_OBJECT
private slots:
    void testGzipCompressDecompress();
    void testZlibCompressDecompress();
    void testIsGzip();
    void testIsZlib();
};

void TestCompressUtils::testGzipCompressDecompress() {
    QByteArray data = "hello world this is a test string for compression";
    QByteArray compressed = sc::CompressUtils::gzipCompress(data);
    QVERIFY(compressed.size() < data.size());
    QByteArray decompressed = sc::CompressUtils::gzipDecompress(compressed);
    QCOMPARE(decompressed, data);
}

void TestCompressUtils::testZlibCompressDecompress() {
    QByteArray data = "hello world this is a test string for compression";
    QByteArray compressed = sc::CompressUtils::zlibCompress(data);
    QVERIFY(compressed.size() < data.size());
    QByteArray decompressed = sc::CompressUtils::zlibDecompress(compressed);
    QCOMPARE(decompressed, data);
}

void TestCompressUtils::testIsGzip() {
    QByteArray data = "hello";
    QByteArray compressed = sc::CompressUtils::gzipCompress(data);
    QVERIFY(sc::CompressUtils::isGzip(compressed));
    QVERIFY(!sc::CompressUtils::isGzip(data));
}

void TestCompressUtils::testIsZlib() {
    QByteArray data = "hello";
    QByteArray compressed = sc::CompressUtils::zlibCompress(data);
    QVERIFY(sc::CompressUtils::isZlib(compressed));
    QVERIFY(!sc::CompressUtils::isZlib(data));
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    TestStringUtils stringUtilsTest;
    QTest::qExec(&stringUtilsTest, argc, argv);

    TestJsonUtils jsonUtilsTest;
    QTest::qExec(&jsonUtilsTest, argc, argv);

    TestDateTimeUtils dateTimeUtilsTest;
    QTest::qExec(&dateTimeUtilsTest, argc, argv);

    TestCryptoUtils cryptoUtilsTest;
    QTest::qExec(&cryptoUtilsTest, argc, argv);

    TestFileUtils fileUtilsTest;
    QTest::qExec(&fileUtilsTest, argc, argv);

    TestXmlUtils xmlUtilsTest;
    QTest::qExec(&xmlUtilsTest, argc, argv);

    TestCompressUtils compressUtilsTest;
    QTest::qExec(&compressUtilsTest, argc, argv);

    return 0;
}

#include "test_utils.moc"