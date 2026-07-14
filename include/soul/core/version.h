#ifndef SOUL_CORE_VERSION_H
#define SOUL_CORE_VERSION_H

#include <string>

namespace sc {

/**
 * @class Version
 * @brief 语义化版本号类（SemVer 2.0）
 *
 * 实现 SemVer 2.0 规范，支持版本号的解析、比较和格式化。
 * 版本号格式：MAJOR.MINOR.PATCH-PRE+BUILD
 *
 * @see https://semver.org/
 */
class Version {
public:
    /**
     * @brief 默认构造函数
     */
    Version() = default;

    /**
     * @brief 构造函数
     * @param major 主版本号
     * @param minor 次版本号
     * @param patch 修订号
     * @param pre 预发布版本标识（可选）
     * @param build 构建元数据（可选）
     */
    Version(int major, int minor, int patch, const std::string& pre = "", const std::string& build = "")
        : m_major(major), m_minor(minor), m_patch(patch), m_pre(pre), m_build(build) {}

    /**
     * @brief 获取主版本号
     * @return 主版本号
     */
    int major() const { return m_major; }

    /**
     * @brief 获取次版本号
     * @return 次版本号
     */
    int minor() const { return m_minor; }

    /**
     * @brief 获取修订号
     * @return 修订号
     */
    int patch() const { return m_patch; }

    /**
     * @brief 小于比较运算符
     * @param other 另一个版本号
     * @return 如果当前版本小于 other 返回 true
     */
    bool operator<(const Version& other) const;

    /**
     * @brief 小于等于比较运算符
     * @param other 另一个版本号
     * @return 如果当前版本小于等于 other 返回 true
     */
    bool operator<=(const Version& other) const;

    /**
     * @brief 大于比较运算符
     * @param other 另一个版本号
     * @return 如果当前版本大于 other 返回 true
     */
    bool operator>(const Version& other) const;

    /**
     * @brief 大于等于比较运算符
     * @param other 另一个版本号
     * @return 如果当前版本大于等于 other 返回 true
     */
    bool operator>=(const Version& other) const;

    /**
     * @brief 等于比较运算符
     * @param other 另一个版本号
     * @return 如果当前版本等于 other 返回 true
     */
    bool operator==(const Version& other) const;

    /**
     * @brief 不等于比较运算符
     * @param other 另一个版本号
     * @return 如果当前版本不等于 other 返回 true
     */
    bool operator!=(const Version& other) const;

    /**
     * @brief 转换为字符串表示
     * @return 格式化的版本号字符串
     */
    std::string toString() const;

    /**
     * @brief 从字符串解析版本号
     * @param versionStr 版本号字符串
     * @return 解析后的 Version 对象
     */
    static Version parse(const std::string& versionStr);

private:
    int m_major = 0;
    int m_minor = 0;
    int m_patch = 0;
    std::string m_pre;
    std::string m_build;
};

}

#endif
