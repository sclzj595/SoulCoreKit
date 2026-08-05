#include "soul/core/banner.h"
#include <QFile>
#include <QTextStream>

namespace sc {

void Banner::print(const std::string& version) {
    printCustom(DEFAULT, version);
}

void Banner::printFromFile(const std::string& filePath, const std::string& version) {
    QFile file(QString::fromStdString(filePath));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        std::string content = stream.readAll().toStdString();
        file.close();
        printCustom(content, version);
    } else {
        // 文件不存在时回退到默认横幅
        print(version);
    }
}

void Banner::printCustom(const std::string& bannerText, const std::string& version) {
    std::cout << std::endl;
    std::cout << bannerText;
    printVersionLine(version);
    std::cout << std::endl;
}

void Banner::printVersionLine(const std::string& version) {
    std::cout << " :: SoulCoreKit :: v" << version << " :: SpringBoot-style CS Architecture Scaffold"
              << std::endl;
}

} // namespace sc