#ifndef SOUL_CORE_TIME_H
#define SOUL_CORE_TIME_H

#include <string>
#include <chrono>

namespace sc {

/**
 * @class Time
 * @brief 时间工具类，提供时间戳、格式化和时区转换功能
 *
 * Time 类封装了常用的时间操作，包括：
 * - 获取当前时间戳
 * - 时间戳格式化（支持自定义格式）
 * - 字符串解析为时间戳
 * - 本地时间/UTC时间转换
 *
 * @note 所有时间戳均使用毫秒精度
 */
class Time {
public:
    /**
     * @brief 时间戳类型（毫秒精度）
     */
    using Timestamp = std::chrono::milliseconds;

    /**
     * @brief 获取当前时间戳
     * @return 当前时间的毫秒时间戳
     */
    static Timestamp now();

    /**
     * @brief 获取当前时间的格式化字符串
     * @param format 时间格式，默认为 "yyyy-MM-dd HH:mm:ss.zzz"
     * @return 格式化的时间字符串
     */
    static std::string nowString(const std::string& format = "yyyy-MM-dd HH:mm:ss.zzz");

    /**
     * @brief 将时间戳格式化为字符串
     * @param timestamp 时间戳
     * @param format 时间格式，默认为 "yyyy-MM-dd HH:mm:ss.zzz"
     * @return 格式化的时间字符串
     */
    static std::string format(Timestamp timestamp, const std::string& format = "yyyy-MM-dd HH:mm:ss.zzz");

    /**
     * @brief 将字符串解析为时间戳
     * @param timeStr 时间字符串
     * @param format 时间格式，默认为 "yyyy-MM-dd HH:mm:ss"
     * @return 解析后的时间戳
     */
    static Timestamp parse(const std::string& timeStr, const std::string& format = "yyyy-MM-dd HH:mm:ss");

    /**
     * @brief 将时间戳转换为本地时间字符串
     * @param timestamp 时间戳
     * @return 本地时间字符串
     */
    static std::string toLocalTime(Timestamp timestamp);

    /**
     * @brief 将时间戳转换为UTC时间字符串
     * @param timestamp 时间戳
     * @return UTC时间字符串
     */
    static std::string toUtcTime(Timestamp timestamp);

    /**
     * @brief 从秒数创建时间戳
     * @param seconds 秒数
     * @return 时间戳
     */
    static Timestamp fromSeconds(int64_t seconds);

    /**
     * @brief 从毫秒数创建时间戳
     * @param ms 毫秒数
     * @return 时间戳
     */
    static Timestamp fromMilliseconds(int64_t ms);

    /**
     * @brief 将时间戳转换为秒数
     * @param timestamp 时间戳
     * @return 秒数
     */
    static int64_t toSeconds(Timestamp timestamp);

    /**
     * @brief 将时间戳转换为毫秒数
     * @param timestamp 时间戳
     * @return 毫秒数
     */
    static int64_t toMilliseconds(Timestamp timestamp);
};

}

#endif
