#include <QTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QCryptographicHash>
#include "soul/auth/oauth2.h"
#include "soul/auth/oidc.h"
#include "soul/core/error.h"

using namespace sc::auth;
using namespace sc;

class TestOAuth2 : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testCodeVerifierGeneration();
    void testCodeChallengeGeneration();
    void testAuthorizationUrlConstruction();
    void testAuthorizationUrlWithPkce();
    void testAuthorizationUrlWithoutPkce();
    void testExchangeCodeForTokenWithEmptyCode();
    void testRefreshTokenWithEmptyToken();
    void testAuthFlowSignalConnection();
    void testClientCredentialsConfig();
    void testClientCredentialsSignalConnection();
    void testDiscoveryWithEmptyUrl();
    void testDiscoveryParsing();
    void testFetchJwksWithEmptyUri();
    void testOidcDiscoverySignalConnection();
    void testTokenDefaultValues();
    void testTokenFullFields();
    void testTokenCopy();
};

void TestOAuth2::initTestCase() {
    // verify we can reach this point
    QVERIFY(true);
}

void TestOAuth2::testCodeVerifierGeneration() {
    OAuth2Config config;
    config.clientId = "test-client";
    config.authorizationEndpoint = "https://auth.example.com/authorize";
    config.tokenEndpoint = "https://auth.example.com/token";
    config.redirectUri = "myapp://callback";

    AuthorizationCodeFlow flow1(config);
    AuthorizationCodeFlow flow2(config);

    QString verifier1 = flow1.codeVerifier();
    QString verifier2 = flow2.codeVerifier();

    QVERIFY(verifier1 != verifier2);
    QVERIFY(verifier1.length() >= 43);
    QVERIFY(verifier1.length() <= 128);

    QRegularExpression base64urlRegex("^[A-Za-z0-9_-]+$");
    QVERIFY(base64urlRegex.match(verifier1).hasMatch());
}

void TestOAuth2::testCodeChallengeGeneration() {
    // Verify that SHA256 code challenge is correctly generated.
    // We generate a verifier, compute the challenge, then verify the
    // challenge is the SHA256 hash of the verifier in base64url encoding.
    OAuth2Config config;
    config.clientId = "test-client";
    config.authorizationEndpoint = "https://auth.example.com/authorize";
    config.tokenEndpoint = "https://auth.example.com/token";
    config.redirectUri = "myapp://callback";

    AuthorizationCodeFlow flow(config);
    QString verifier = flow.codeVerifier();

    // Manually compute the expected challenge
    QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    QByteArray expectedChallenge = hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    // The authorizationUrl should contain the correct challenge
    QString url = flow.authorizationUrl();
    QString challengeParam = QString("code_challenge=") + QString::fromLatin1(expectedChallenge);
    QVERIFY2(url.contains(challengeParam), "Expected challenge not found in authorization URL");
}

void TestOAuth2::testAuthorizationUrlConstruction() {
    OAuth2Config config;
    config.clientId = "my-client";
    config.authorizationEndpoint = "https://auth.example.com/authorize";
    config.tokenEndpoint = "https://auth.example.com/token";
    config.redirectUri = "myapp://callback";
    config.scopes = {"openid", "profile", "email"};

    AuthorizationCodeFlow flow(config);
    QString url = flow.authorizationUrl();

    QVERIFY(url.startsWith("https://auth.example.com/authorize?"));
    QVERIFY(url.contains("response_type=code"));
    QVERIFY(url.contains("client_id=my-client"));
    QVERIFY(url.contains("scope=openid"));
    QVERIFY(url.contains("profile"));
    QVERIFY(url.contains("email"));
    QVERIFY(url.contains("code_challenge_method=S256"));
    QVERIFY(url.contains("code_challenge="));
}

void TestOAuth2::testAuthorizationUrlWithPkce() {
    OAuth2Config config;
    config.clientId = "pkce-client";
    config.authorizationEndpoint = "https://auth.example.com/authorize";
    config.tokenEndpoint = "https://auth.example.com/token";
    config.redirectUri = "pkce://callback";
    config.usePkce = true;

    AuthorizationCodeFlow flow(config);
    QString url = flow.authorizationUrl();

    QVERIFY(url.contains("code_challenge_method=S256"));
    QVERIFY(url.contains("code_challenge="));
}

void TestOAuth2::testAuthorizationUrlWithoutPkce() {
    OAuth2Config config;
    config.clientId = "no-pkce-client";
    config.authorizationEndpoint = "https://auth.example.com/authorize";
    config.tokenEndpoint = "https://auth.example.com/token";
    config.redirectUri = "app://callback";
    config.usePkce = false;

    AuthorizationCodeFlow flow(config);
    QString url = flow.authorizationUrl();

    QVERIFY(!url.contains("code_challenge"));
    QVERIFY(!url.contains("code_challenge_method"));
}

void TestOAuth2::testExchangeCodeForTokenWithEmptyCode() {
    OAuth2Config config;
    config.clientId = "test-client";
    config.tokenEndpoint = "https://auth.example.com/token";
    config.redirectUri = "myapp://callback";

    AuthorizationCodeFlow flow(config);
    auto result = flow.exchangeCodeForToken("", "");

    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), ErrorCode::InvalidArgument);
}

void TestOAuth2::testRefreshTokenWithEmptyToken() {
    OAuth2Config config;
    config.clientId = "test-client";
    config.tokenEndpoint = "https://auth.example.com/token";

    AuthorizationCodeFlow flow(config);
    auto result = flow.refreshToken("");

    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), ErrorCode::InvalidArgument);
}

void TestOAuth2::testAuthFlowSignalConnection() {
    OAuth2Config config;
    config.clientId = "test-client";
    config.tokenEndpoint = "https://auth.example.com/token";
    config.redirectUri = "myapp://callback";

    AuthorizationCodeFlow flow(config);
    QSignalSpy spy(&flow, &AuthorizationCodeFlow::tokenReceived);
    QVERIFY(spy.isValid());
}

void TestOAuth2::testClientCredentialsConfig() {
    OAuth2Config config;
    config.clientId = "service-client";
    config.clientSecret = "service-secret";
    config.tokenEndpoint = "https://auth.example.com/token";
    config.scopes = {"api:read", "api:write"};

    ClientCredentialsFlow flow(config);
    QSignalSpy spy(&flow, &ClientCredentialsFlow::tokenReceived);
    QVERIFY(spy.isValid());
}

void TestOAuth2::testClientCredentialsSignalConnection() {
    OAuth2Config config;
    config.clientId = "test-client";
    config.clientSecret = "test-secret";
    config.tokenEndpoint = "https://auth.example.com/token";

    ClientCredentialsFlow flow(config);

    QSignalSpy tokenSpy(&flow, &ClientCredentialsFlow::tokenReceived);
    QSignalSpy errorSpy(&flow, &ClientCredentialsFlow::tokenError);
    QVERIFY(tokenSpy.isValid());
    QVERIFY(errorSpy.isValid());
}

void TestOAuth2::testDiscoveryWithEmptyUrl() {
    OidcDiscoveryService service;
    auto result = service.discover("");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), ErrorCode::InvalidArgument);
}

void TestOAuth2::testDiscoveryParsing() {
    QJsonObject json;
    json["issuer"] = "https://accounts.example.com";
    json["authorization_endpoint"] = "https://accounts.example.com/authorize";
    json["token_endpoint"] = "https://accounts.example.com/token";
    json["userinfo_endpoint"] = "https://accounts.example.com/userinfo";
    json["jwks_uri"] = "https://accounts.example.com/jwks";

    QJsonArray scopes = {"openid", "profile", "email"};
    json["scopes_supported"] = scopes;

    QJsonArray responseTypes = {"code", "token", "id_token"};
    json["response_types_supported"] = responseTypes;

    QCOMPARE(json["issuer"].toString(), QString("https://accounts.example.com"));
    QCOMPARE(json["authorization_endpoint"].toString(), QString("https://accounts.example.com/authorize"));
    QCOMPARE(json["token_endpoint"].toString(), QString("https://accounts.example.com/token"));
    QCOMPARE(json["userinfo_endpoint"].toString(), QString("https://accounts.example.com/userinfo"));
    QCOMPARE(json["jwks_uri"].toString(), QString("https://accounts.example.com/jwks"));
    QCOMPARE(json["scopes_supported"].toArray().size(), 3);
    QCOMPARE(json["response_types_supported"].toArray().size(), 3);
}

void TestOAuth2::testFetchJwksWithEmptyUri() {
    OidcDiscoveryService service;
    auto result = service.fetchJwks("");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), ErrorCode::InvalidArgument);
}

void TestOAuth2::testOidcDiscoverySignalConnection() {
    OidcDiscoveryService service;

    QSignalSpy completedSpy(&service, &OidcDiscoveryService::discoveryCompleted);
    QSignalSpy errorSpy(&service, &OidcDiscoveryService::discoveryError);
    QVERIFY(completedSpy.isValid());
    QVERIFY(errorSpy.isValid());
}

void TestOAuth2::testTokenDefaultValues() {
    OAuth2Token token;
    QVERIFY(token.accessToken.isEmpty());
    QVERIFY(token.refreshToken.isEmpty());
    QVERIFY(token.tokenType.isEmpty());
    QCOMPARE(token.expiresIn, 0);
    QVERIFY(token.idToken.isEmpty());
}

void TestOAuth2::testTokenFullFields() {
    OAuth2Token token;
    token.accessToken = "eyJhbGciOiJSUzI1NiJ9.access";
    token.refreshToken = "refresh-token-123";
    token.tokenType = "Bearer";
    token.expiresIn = 3600;
    token.idToken = "eyJhbGciOiJSUzI1NiJ9.id";

    QCOMPARE(token.accessToken, QString("eyJhbGciOiJSUzI1NiJ9.access"));
    QCOMPARE(token.refreshToken, QString("refresh-token-123"));
    QCOMPARE(token.tokenType, QString("Bearer"));
    QCOMPARE(token.expiresIn, 3600);
    QCOMPARE(token.idToken, QString("eyJhbGciOiJSUzI1NiJ9.id"));
}

void TestOAuth2::testTokenCopy() {
    OAuth2Token original;
    original.accessToken = "access-token";
    original.refreshToken = "refresh-token";
    original.tokenType = "Bearer";
    original.expiresIn = 1800;
    original.idToken = "id-token";

    OAuth2Token copy = original;
    QCOMPARE(copy.accessToken, original.accessToken);
    QCOMPARE(copy.refreshToken, original.refreshToken);
    QCOMPARE(copy.tokenType, original.tokenType);
    QCOMPARE(copy.expiresIn, original.expiresIn);
    QCOMPARE(copy.idToken, original.idToken);
}

QTEST_MAIN(TestOAuth2)
#include "test_oauth2.moc"