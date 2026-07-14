#include <QTest>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include "soul/network/http_request.h"

class TestHttpRequest : public QObject {
    Q_OBJECT

private slots:
    void testUrlBuilding();
    void testQueryParams();
    void testHeaders();
    void testJsonBody();
};

void TestHttpRequest::testUrlBuilding() {
    sc::HttpRequest request(sc::HttpMethod::Get, QUrl("https://api.example.com/users"));
    
    QCOMPARE(request.method(), sc::HttpMethod::Get);
    QCOMPARE(request.url().toString(), QString("https://api.example.com/users"));
}

void TestHttpRequest::testQueryParams() {
    sc::HttpRequest request(sc::HttpMethod::Get, QUrl("https://api.example.com/users"));
    
    request.addParam("page", "1");
    request.addParam("limit", "10");
    request.addParam("sort", "name");

    QUrl builtUrl = request.buildUrl();
    QUrlQuery query(builtUrl);
    QCOMPARE(query.queryItemValue("page"), QString("1"));
    QCOMPARE(query.queryItemValue("limit"), QString("10"));
    QCOMPARE(query.queryItemValue("sort"), QString("name"));
}

void TestHttpRequest::testHeaders() {
    sc::HttpRequest request;
    
    request.addHeader("Authorization", "Bearer token");
    request.addHeader("Content-Type", "application/json");

    auto headers = request.headers();
    QVERIFY(headers.contains("Authorization"));
    QVERIFY(headers.contains("Content-Type"));
    QCOMPARE(headers["Authorization"], QString("Bearer token"));
}

void TestHttpRequest::testJsonBody() {
    sc::HttpRequest request(sc::HttpMethod::Post, QUrl("https://api.example.com/users"));
    
    QJsonObject json;
    json["name"] = "John";
    json["email"] = "john@example.com";
    request.setJsonBody(QJsonDocument(json));

    QVERIFY(request.body().size() > 0);
    QVERIFY(request.headers().contains("Content-Type"));
    QCOMPARE(request.headers()["Content-Type"], QString("application/json"));
}

QTEST_MAIN(TestHttpRequest)
#include "test_http_request.moc"
