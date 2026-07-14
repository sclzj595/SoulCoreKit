#ifndef SOUL_UI_GLASS_EFFECT_CACHE_H
#define SOUL_UI_GLASS_EFFECT_CACHE_H

#include <QPixmap>
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>

namespace sc {

class GlassEffectCache {
public:
    struct BlurContext {
        QGraphicsScene scene;
        QGraphicsBlurEffect blurEffect;
        QGraphicsPixmapItem* pixmapItem = nullptr;
        QPixmap cachedResult;
        QSize lastSize;
        int lastBlurRadius = -1;

        BlurContext() {
            blurEffect.setBlurHints(QGraphicsBlurEffect::PerformanceHint);
        }

        ~BlurContext() {
            if (pixmapItem) {
                delete pixmapItem;
            }
        }

        QPixmap apply(const QPixmap& source, int blurRadius) {
            if (source.isNull()) return QPixmap();

            bool needUpdate = cachedResult.isNull() ||
                              lastSize != source.size() ||
                              lastBlurRadius != blurRadius;

            if (!needUpdate) {
                return cachedResult;
            }

            if (pixmapItem) {
                scene.removeItem(pixmapItem);
                delete pixmapItem;
            }

            blurEffect.setBlurRadius(blurRadius);
            pixmapItem = scene.addPixmap(source);
            pixmapItem->setGraphicsEffect(&blurEffect);

            cachedResult = QPixmap(source.size());
            cachedResult.fill(Qt::transparent);

            QPainter painter(&cachedResult);
            scene.render(&painter);

            lastSize = source.size();
            lastBlurRadius = blurRadius;

            return cachedResult;
        }

        void invalidate() {
            cachedResult = QPixmap();
            lastSize = QSize();
            lastBlurRadius = -1;
        }
    };

    static GlassEffectCache& instance() {
        static GlassEffectCache cache;
        return cache;
    }

    QPixmap blur(const QPixmap& source, int blurRadius) {
        std::lock_guard<std::mutex> lock(m_mutex);

        size_t key = hashKey(source.size(), blurRadius);
        auto it = m_contexts.find(key);

        if (it == m_contexts.end()) {
            it = m_contexts.emplace(key, std::make_unique<BlurContext>()).first;
        }

        return it->second->apply(source, blurRadius);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_contexts.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_contexts.size();
    }

private:
    GlassEffectCache() = default;
    ~GlassEffectCache() = default;

    GlassEffectCache(const GlassEffectCache&) = delete;
    GlassEffectCache& operator=(const GlassEffectCache&) = delete;

    size_t hashKey(const QSize& size, int blurRadius) const {
        return static_cast<size_t>(size.width()) * 31 +
               static_cast<size_t>(size.height()) * 31 +
               static_cast<size_t>(blurRadius);
    }

    mutable std::mutex m_mutex;
    std::unordered_map<size_t, std::unique_ptr<BlurContext>> m_contexts;
};

}

#endif
