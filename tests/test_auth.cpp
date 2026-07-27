#include <QTest>
#include "soul/auth/auth_manager.h"

using namespace sc;

class TestAuthManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testLogin();
    void testLogout();
    void testIsAuthenticated();
    void testCurrentUser();
    void testAccessToken();
    void testRefreshToken();
    void testHasPermission();
    void testHasRole();
    void testAuthStateCallback();
    void testLoadAndSaveAuthState();
};

void TestAuthManager::initTestCase() {
    AuthManager::instance().init();
}

void TestAuthManager::cleanupTestCase() {
    AuthManager::instance().shutdown();
}

void TestAuthManager::testLogin() {
    AuthManager::instance().logout();
    
    auto result = AuthManager::instance().login("testuser", "password");
    QVERIFY(result.isOk());
    
    QCOMPARE(result.unwrap().username, QString("testuser"));
    QCOMPARE(result.unwrap().role, QString("admin"));
    QVERIFY(!result.unwrap().token.isEmpty());
}

void TestAuthManager::testLogout() {
    AuthManager::instance().login("testuser", "password");
    QVERIFY(AuthManager::instance().isAuthenticated());
    
    AuthManager::instance().logout();
    QVERIFY(!AuthManager::instance().isAuthenticated());
}

void TestAuthManager::testIsAuthenticated() {
    AuthManager::instance().logout();
    QVERIFY(!AuthManager::instance().isAuthenticated());
    
    AuthManager::instance().login("testuser", "password");
    QVERIFY(AuthManager::instance().isAuthenticated());
}

void TestAuthManager::testCurrentUser() {
    AuthManager::instance().login("testuser", "password");
    
    auto user = AuthManager::instance().currentUser();
    QCOMPARE(user.username, QString("testuser"));
    QCOMPARE(user.role, QString("admin"));
    QVERIFY(!user.token.isEmpty());
}

void TestAuthManager::testAccessToken() {
    AuthManager::instance().login("testuser", "password");
    QString token = AuthManager::instance().accessToken();
    QVERIFY(!token.isEmpty());
}

void TestAuthManager::testRefreshToken() {
    AuthManager::instance().login("testuser", "password");
    
    QString originalToken = AuthManager::instance().accessToken();
    auto result = AuthManager::instance().refreshToken();
    
    QVERIFY(result.isOk());
    QString newToken = result.unwrap();
    QVERIFY(newToken != originalToken);
    QCOMPARE(AuthManager::instance().accessToken(), newToken);
}

void TestAuthManager::testHasPermission() {
    AuthManager::instance().login("testuser", "password");
    QVERIFY(AuthManager::instance().hasPermission("admin"));
    QVERIFY(AuthManager::instance().hasPermission("any"));
    
    AuthManager::instance().logout();
    QVERIFY(!AuthManager::instance().hasPermission("admin"));
}

void TestAuthManager::testHasRole() {
    AuthManager::instance().login("testuser", "password");
    QVERIFY(AuthManager::instance().hasRole("admin"));
    QVERIFY(!AuthManager::instance().hasRole("user"));
}

void TestAuthManager::testAuthStateCallback() {
    bool callbackCalled = false;
    bool lastState = false;
    
    AuthManager::instance().setAuthStateCallback([&callbackCalled, &lastState](bool authenticated) {
        callbackCalled = true;
        lastState = authenticated;
    });
    
    AuthManager::instance().login("testuser", "password");
    QVERIFY(callbackCalled);
    QVERIFY(lastState);
    
    callbackCalled = false;
    AuthManager::instance().logout();
    QVERIFY(callbackCalled);
    QVERIFY(!lastState);
}

void TestAuthManager::testLoadAndSaveAuthState() {
    // 验证 saveAuthState + loadAuthState 往返:登录后保存,再加载应保持登录态
    AuthManager::instance().login("testuser", "password");
    auto saved = AuthManager::instance().saveAuthState();
    QVERIFY(saved.isOk());
    QVERIFY(AuthManager::instance().isAuthenticated());

    // loadAuthState 在已登录状态下应保持登录态并恢复同一用户
    auto loaded = AuthManager::instance().loadAuthState();
    QVERIFY(loaded.isOk());
    QVERIFY(AuthManager::instance().isAuthenticated());
    QCOMPARE(AuthManager::instance().currentUser().username, QString("testuser"));

    // V-6 安全修复:logout() 必须清除 storage,防止重启后恢复登录态
    AuthManager::instance().logout();
    QVERIFY(!AuthManager::instance().isAuthenticated());

    // logout 后 storage 已清空,loadAuthState 应返回 Ok 但不恢复登录态
    auto loadedAfterLogout = AuthManager::instance().loadAuthState();
    QVERIFY(loadedAfterLogout.isOk());
    QVERIFY(!AuthManager::instance().isAuthenticated());
}

QTEST_MAIN(TestAuthManager)
#include "test_auth.moc"