// ============================================================================
// test_config_provider.cpp — ConfigProvider + Snapshot 测试 [v2.9.0]
// ============================================================================

#include <QtTest>
#include <QHash>
#include <QVariant>
#include <memory>
#include "soul/configuration/iconfig_provider.h"

using namespace sc;

class TestConfigProvider : public QObject {
    Q_OBJECT

private:
    // Mock Provider: 总是返回预设值
    class MockProvider : public IConfigProvider {
    public:
        MockProvider(const QString& n, int p, QHash<QString,QVariant> vals,
                     bool shouldFail = false)
            : m_name(n), m_prio(p), m_values(std::move(vals)), m_fail(shouldFail) {}

        Result<ConfigSnapshot> load() override {
            if (m_fail) {
                return Result<ConfigSnapshot>::err(
                    Error(ErrorCode::InternalError, "Mock failure"));
            }
            auto snap = ConfigSnapshot(m_values);
            snap.setSourceName(m_name);
            return Result<ConfigSnapshot>::ok(std::move(snap));
        }
        std::string name() const override { return m_name.toStdString(); }
        int priority() const override { return m_prio; }

    private:
        QString m_name;
        int m_prio;
        QHash<QString,QVariant> m_values;
        bool m_fail;
    };

private slots:
    // 1. Snapshot 基本读写
    void testSnapshotBasic() {
        QHash<QString, QVariant> vals;
        vals["server.port"] = 8080;
        vals["server.host"] = "0.0.0.0";
        vals["debug"] = true;
        vals["timeout"] = 30.5;

        ConfigSnapshot snap(vals);
        QCOMPARE(snap.getInt("server.port").value(), 8080);
        QCOMPARE(snap.getString("server.host").value(), QString("0.0.0.0"));
        QCOMPARE(snap.getBool("debug").value(), true);
        QVERIFY(qFuzzyCompare(snap.getDouble("timeout").value(), 30.5));
        QVERIFY(snap.contains("server.port"));
        QVERIFY(!snap.contains("nonexistent"));
        QCOMPARE(snap.size(), size_t(4));
    }

    // 2. Snapshot getOr 默认值
    void testSnapshotGetOr() {
        QHash<QString, QVariant> vals;
        vals["key"] = "value";

        ConfigSnapshot snap(vals);
        QCOMPARE(snap.getStringOr("key", "default"), QString("value"));
        QCOMPARE(snap.getStringOr("missing", "default"), QString("default"));
        QCOMPARE(snap.getIntOr("missing", 42), 42);
        QCOMPARE(snap.getBoolOr("missing", true), true);
    }

    // 3. Snapshot merge — 高优先级覆盖低优先级
    void testSnapshotMerge() {
        QHash<QString, QVariant> low;
        low["host"] = "localhost";
        low["port"] = 8080;

        QHash<QString, QVariant> high;
        high["port"] = 9090;   // 覆盖
        high["debug"] = true;  // 新增

        ConfigSnapshot lowSnap(low);
        ConfigSnapshot highSnap(high);

        auto merged = lowSnap.merge(highSnap);

        QCOMPARE(merged.getStringOr("host", ""), QString("localhost"));  // 保留
        QCOMPARE(merged.getIntOr("port", 0), 9090);  // 覆盖
        QCOMPARE(merged.getBoolOr("debug", false), true);  // 新增
    }

    // 4. PriorityConfigChain — 按优先级合并
    void testChainPriority() {
        auto chain = std::make_shared<PriorityConfigChain>();

        QHash<QString, QVariant> low;
        low["host"] = "localhost";
        low["port"] = 8080;
        chain->addProvider(std::make_shared<MockProvider>("Low", 0, low));

        QHash<QString, QVariant> high;
        high["port"] = 9090;
        chain->addProvider(std::make_shared<MockProvider>("High", 100, high));

        auto result = chain->load();
        QVERIFY(result.isOk());

        auto snap = result.unwrap();
        QCOMPARE(snap.getStringOr("host", ""), QString("localhost"));
        QCOMPARE(snap.getIntOr("port", 0), 9090);  // 高优先级覆盖
    }

    // 5. Chain — 空 Provider 列表
    void testChainEmpty() {
        auto chain = std::make_shared<PriorityConfigChain>();
        auto result = chain->load();
        QVERIFY(result.isOk());  // 空列表返回 Ok
    }

    // 6. Chain — Provider 失败传播
    void testChainProviderFailure() {
        auto chain = std::make_shared<PriorityConfigChain>();

        QHash<QString, QVariant> vals;
        vals["key"] = "value";
        chain->addProvider(std::make_shared<MockProvider>("Ok", 50, vals));
        chain->addProvider(std::make_shared<MockProvider>("Fail", 0, vals, true));

        auto result = chain->load();
        QVERIFY(result.isErr());  // 非 Remote Provider 失败应传播
    }

    // 7. Chain — Remote Provider 失败降级
    void testChainRemoteDegradation() {
        auto chain = std::make_shared<PriorityConfigChain>();

        QHash<QString, QVariant> localVals;
        localVals["host"] = "localhost";
        chain->addProvider(std::make_shared<MockProvider>("Local", 50, localVals));

        QHash<QString, QVariant> remoteVals;
        remoteVals["host"] = "remote-host";
        chain->addProvider(std::make_shared<MockProvider>("Remote", ConfigPriority::Remote, remoteVals, true));

        auto result = chain->load();
        QVERIFY(result.isOk());  // Remote 失败应降级

        auto snap = result.unwrap();
        QCOMPARE(snap.getStringOr("host", ""), QString("localhost"));  // 使用本地配置
    }

    // 8. tryReload — 重新加载 Remote Provider
    void testChainTryReload() {
        auto chain = std::make_shared<PriorityConfigChain>();

        QHash<QString, QVariant> localVals;
        localVals["host"] = "localhost";
        localVals["port"] = 8080;
        chain->addProvider(std::make_shared<MockProvider>("Local", 50, localVals));

        QHash<QString, QVariant> remoteVals;
        remoteVals["port"] = 9090;
        chain->addProvider(std::make_shared<MockProvider>("Remote", ConfigPriority::Remote, remoteVals));

        // 首次加载
        auto result = chain->load();
        QVERIFY(result.isOk());
        QCOMPARE(result.unwrap().getIntOr("port", 0), 9090);  // Remote 覆盖

        // 模拟 Remote 失效后 reload
        auto reloadResult = chain->tryReload();
        QVERIFY(reloadResult.isOk());
        QCOMPARE(reloadResult.unwrap().getIntOr("port", 0), 9090);  // 保持旧值
    }

    // 9. Snapshot 不可变 — merge 返回新对象
    void testSnapshotImmutability() {
        QHash<QString, QVariant> a_vals;
        a_vals["key"] = "A";

        QHash<QString, QVariant> b_vals;
        b_vals["key"] = "B";

        ConfigSnapshot a(a_vals);
        ConfigSnapshot b(b_vals);

        auto merged = a.merge(b);
        QCOMPARE(merged.getStringOr("key", ""), QString("B"));  // merged 是 B
        QCOMPARE(a.getStringOr("key", ""), QString("A"));      // a 不变
        QCOMPARE(b.getStringOr("key", ""), QString("B"));      // b 不变
    }

    // 10. currentSnapshot 返回最新加载的快照
    void testChainCurrentSnapshot() {
        auto chain = std::make_shared<PriorityConfigChain>();

        QHash<QString, QVariant> vals;
        vals["version"] = 1;
        chain->addProvider(std::make_shared<MockProvider>("Test", 0, vals));

        QVERIFY(chain->load().isOk());
        QCOMPARE(chain->currentSnapshot().getIntOr("version", 0), 1);
    }
};

QTEST_MAIN(TestConfigProvider)
#include "test_config_provider.moc"
