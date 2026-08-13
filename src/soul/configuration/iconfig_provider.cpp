// ============================================================================
// iconfig_provider.cpp — ConfigProvider + PriorityConfigChain 实现 [v2.9.0]
// ============================================================================

#include "soul/configuration/iconfig_provider.h"
#include <algorithm>

namespace sc {

// ============================================================================
// PriorityConfigChain 实现
// ============================================================================

void PriorityConfigChain::addProvider(std::shared_ptr<IConfigProvider> provider) {
    if (!provider) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_providers.push_back(std::move(provider));

    // 按优先级降序排列 (高优先级在前，先加载)
    std::sort(m_providers.begin(), m_providers.end(),
        [](const auto& a, const auto& b) {
            return a->priority() > b->priority();
        });
}

Result<ConfigSnapshot> PriorityConfigChain::load() {
    std::vector<std::shared_ptr<IConfigProvider>> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        snapshot = m_providers;
    }

    if (snapshot.empty()) {
        return Result<ConfigSnapshot>::ok(ConfigSnapshot());
    }

    ConfigSnapshot merged;
    bool hasAtLeastOne = false;

    for (auto& provider : snapshot) {
        auto result = provider->load();

        if (result.isErr()) {
            auto& err = result.unwrapErr();

            // Remote Provider 失败 → 降级 (非关键)
            if (provider->priority() == ConfigPriority::Remote) {
                // 跳过，继续使用已有配置
                continue;
            }

            // 其他 Provider 失败 → 致命错误
            return Result<ConfigSnapshot>::err(
                Error(ErrorCode::InternalError,
                    QString("ConfigProvider '%1' failed: %2")
                        .arg(QString::fromStdString(provider->name()))
                        .arg(err.message()),
                    std::make_shared<Error>(err))
            );
        }

        auto snap = result.unwrap();
        snap.setSourceName(QString::fromStdString(provider->name()));

        if (!hasAtLeastOne) {
            merged = std::move(snap);
            hasAtLeastOne = true;
        } else {
            // v3.0.0: 修复优先级覆盖方向
            // Provider 已按 priority 降序排列 (高优先级在前)。
            // merge(higher) 的语义是 higher 参数覆盖 this。
            // 因此要让已累积的高优先级 merged 覆盖新来的低优先级 snap,
            // 必须调用 snap.merge(merged) (而非 merged.merge(snap))。
            merged = snap.merge(merged);
        }
    }

    if (!hasAtLeastOne) {
        // 所有 Provider 都失败了 (例如全部是 Remote 且不可用)
        return Result<ConfigSnapshot>::err(
            Error(ErrorCode::InternalError, "No config provider loaded successfully")
        );
    }

    merged.setVersion(QString("chain-%1").arg(snapshot.size()));
    merged.setLoadedAt(std::chrono::system_clock::now());

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentSnapshot = merged;
    }

    return Result<ConfigSnapshot>::ok(std::move(merged));
}

Result<ConfigSnapshot> PriorityConfigChain::tryReload() {
    std::vector<std::shared_ptr<IConfigProvider>> snapshot;
    ConfigSnapshot oldSnapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        snapshot = m_providers;
        oldSnapshot = m_currentSnapshot;
    }

    // 只重新加载 Remote Provider (文件和默认值不变)
    ConfigSnapshot newMerged = oldSnapshot;

    for (auto& provider : snapshot) {
        if (provider->priority() != ConfigPriority::Remote) {
            continue;  // 只 reload Remote
        }

        auto result = provider->load();
        if (result.isErr()) {
            // 远程配置加载失败 → 保持旧配置
            continue;
        }

        newMerged = newMerged.merge(result.unwrap());
    }

    newMerged.setVersion(QString("reload-%1").arg(
        std::chrono::system_clock::now().time_since_epoch().count()));
    newMerged.setLoadedAt(std::chrono::system_clock::now());

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentSnapshot = newMerged;
    }

    return Result<ConfigSnapshot>::ok(std::move(newMerged));
}

ConfigSnapshot PriorityConfigChain::currentSnapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSnapshot;
}

} // namespace sc
