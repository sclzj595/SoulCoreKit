#ifndef SOUL_CORE_BANNER_H
#define SOUL_CORE_BANNER_H

// ============================================================================
// banner.h — 启动横幅 [v2.0.0]
// ============================================================================
//
// 对标 SpringBoot 的 Banner 机制。
// 支持自定义 ASCII Art 横幅，默认显示 SoulCoreKit 版本信息。
//
// 用法:
//   Banner::print();
//   Banner::print("banner.txt");  // 从文件读取自定义横幅

#include <string>
#include <iostream>

namespace sc {

class Banner {
public:
    /// @brief SoulCoreKit 默认横幅
    static constexpr const char* DEFAULT =
R"(  ____             _        ____             _  __ _   _
 / ___|  ___  _   _| | ___  / ___|___  _ __  | |/ /(_) |_ ___
 \___ \ / _ \| | | | |/ _ \| |   / _ \| '__| | ' / | | __/ _ \
  ___) | (_) | |_| | | (_) | |__| (_) | |    | . \ | | ||  __/
 |____/ \___/ \__,_|_|\___/ \____\___/|_|    |_|\_\|_|\__\___|
)";

    /// @brief SpringBoot 风格版本信息
    static constexpr const char* SPRING_BOOT_STYLE =
R"(  .   ____          _            __ _ _
 /\\ / ___'_ __ _ _(_)_ __  __ _ \ \ \ \
( ( )\___ | '_ | '_| | '_ \/ _` | \ \ \ \
 \\/  ___)| |_)| | | | | || (_| |  ) ) ) )
  '  |____| .__|_| |_|_| |_\__, | / / / /
 =========|_|==============|___/=/_/_/_/
)";

    /// @brief 打印默认横幅
    /// @param version 版本号
    static void print(const std::string& version = "2.0.0");

    /// @brief 打印自定义横幅(从文件)
    /// @param filePath 横幅文件路径
    /// @param version 版本号
    static void printFromFile(const std::string& filePath, const std::string& version = "2.0.0");

    /// @brief 打印自定义横幅(从字符串)
    /// @param bannerText 横幅文本
    /// @param version 版本号
    static void printCustom(const std::string& bannerText, const std::string& version = "2.0.0");

private:
    static void printVersionLine(const std::string& version);
};

} // namespace sc

#endif // SOUL_CORE_BANNER_H