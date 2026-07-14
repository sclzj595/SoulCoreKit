#ifndef SOUL_CORE_UUID_H
#define SOUL_CORE_UUID_H

#include <string>

namespace sc {

/**
 * @class Uuid
 * @brief UUID生成与验证工具类
 *
 * Uuid 类提供UUID相关的工具方法，包括：
 * - 生成UUID字符串
 * - 验证UUID格式是否有效
 * - 格式化UUID字符串
 *
 * @note 基于Qt的QUuid实现，支持标准UUID格式
 */
class Uuid {
public:
    /**
     * @brief 生成UUID字符串
     * @return 新生成的UUID字符串（格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx）
     */
    static std::string generate();

    /**
     * @brief 验证UUID格式是否有效
     * @param uuid 待验证的UUID字符串
     * @return 如果格式有效返回 true，否则返回 false
     */
    static bool isValid(const std::string& uuid);

    /**
     * @brief 格式化UUID字符串
     * @param uuid 待格式化的UUID字符串
     * @return 格式化后的UUID字符串
     */
    static std::string format(const std::string& uuid);
};

}

#endif
