#ifndef SOUL_CACHE_SIZE_ESTIMATOR_H
#define SOUL_CACHE_SIZE_ESTIMATOR_H

#include <cstddef>
#include <string>
#include <vector>
#include <QString>
#include <QByteArray>

namespace sc {
namespace cache {

/**
 * @brief 大小估算器(类型无关)
 *
 * 为 MemoryCache 提供统一的内存占用估算入口。默认实现返回 sizeof(V);
 * 用户可为复杂类型提供特化以获得更准确的估算,避免 OOM。
 *
 * @par 自定义特化示例
 * @code
 * template<>
 * struct SizeEstimator<MyStruct> {
 *     static std::size_t estimate(const MyStruct& v) {
 *         return sizeof(MyStruct) + v.buffer.size();
 *     }
 * };
 * @endcode
 *
 * @see MemoryCache::estimateSize
 */
template<typename V>
struct SizeEstimator {
    static std::size_t estimate(const V& /*value*/) noexcept {
        return sizeof(V);
    }
};

// 常见标准库类型的特化

template<>
struct SizeEstimator<std::string> {
    static std::size_t estimate(const std::string& v) noexcept {
        return sizeof(std::string) + v.size();
    }
};

template<typename T, typename Alloc>
struct SizeEstimator<std::vector<T, Alloc>> {
    static std::size_t estimate(const std::vector<T, Alloc>& v) noexcept {
        return sizeof(std::vector<T, Alloc>) + v.size() * sizeof(T);
    }
};

// Qt 类型特化

template<>
struct SizeEstimator<QString> {
    static std::size_t estimate(const QString& v) noexcept {
        // QString 内部使用隐式共享,实际占用约 sizeof + 字节数
        return sizeof(QString) + static_cast<std::size_t>(v.size() * sizeof(QChar));
    }
};

template<>
struct SizeEstimator<QByteArray> {
    static std::size_t estimate(const QByteArray& v) noexcept {
        return sizeof(QByteArray) + static_cast<std::size_t>(v.size());
    }
};

} // namespace cache
} // namespace sc

#endif // SOUL_CACHE_SIZE_ESTIMATOR_H
