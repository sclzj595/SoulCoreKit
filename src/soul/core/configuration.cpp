#include "soul/core/configuration.h"
#include <QFile>
#include <QTextStream>
#include <sstream>
#include <algorithm>

namespace sc {

Configuration& Configuration::instance() {
    static Configuration config;
    return config;
}

bool Configuration::loadFromFile(const std::string& filePath) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();
    parseYaml(content.toStdString());
    return true;
}

void Configuration::loadFromString(const std::string& yamlContent) {
    parseYaml(yamlContent);
}

void Configuration::setActiveProfile(const std::string& profile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeProfile = profile;
}

std::string Configuration::activeProfile() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeProfile;
}

bool Configuration::contains(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_values.find(key) != m_values.end();
}

std::vector<std::string> Configuration::keys() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> result;
    result.reserve(m_values.size());
    for (const auto& pair : m_values) {
        result.push_back(pair.first);
    }
    return result;
}

std::map<std::string, std::string> Configuration::all() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::string, std::string> result;
    for (const auto& pair : m_values) {
        result[pair.first] = pair.second.toString().toStdString();
    }
    return result;
}

void Configuration::parseCommandLine(int argc, char* argv[]) {
    // 解析 --key=value 格式的命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
            std::string kv = arg.substr(2);
            auto eqPos = kv.find('=');
            if (eqPos != std::string::npos) {
                std::string key = kv.substr(0, eqPos);
                std::string value = kv.substr(eqPos + 1);
                std::lock_guard<std::mutex> lock(m_mutex);
                // 尝试转换为整数
                try {
                    int intVal = std::stoi(value);
                    m_values[key] = QVariant(intVal);
                } catch (...) {
                    // 尝试转换为布尔值
                    if (value == "true" || value == "True") {
                        m_values[key] = QVariant(true);
                    } else if (value == "false" || value == "False") {
                        m_values[key] = QVariant(false);
                    } else {
                        m_values[key] = QVariant(QString::fromStdString(value));
                    }
                }
            }
        }
    }
}

void Configuration::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values.clear();
    m_activeProfile.clear();
}

void Configuration::storeValue(const std::string& key, const std::string& value) {
    // 尝试解析为整数
    try {
        int intVal = std::stoi(value);
        m_values[key] = QVariant(intVal);
    } catch (...) {
        // 尝试解析为布尔值
        if (value == "true" || value == "True") {
            m_values[key] = QVariant(true);
        } else if (value == "false" || value == "False") {
            m_values[key] = QVariant(false);
        } else {
            m_values[key] = QVariant(QString::fromStdString(value));
        }
    }
}

void Configuration::parseYaml(const std::string& content, const std::string& prefix) {
    std::istringstream stream(content);
    std::string line;
    std::vector<std::string> prefixStack;
    int listIndex = 0;  // 当前列表项索引 [v2.0.0]

    // 如果已有 prefix,预设前缀层级
    if (!prefix.empty()) {
        std::string part;
        std::istringstream ps(prefix);
        while (std::getline(ps, part, '.')) {
            prefixStack.push_back(part);
        }
    }

    while (std::getline(stream, line)) {
        // 跳过空行和注释
        if (line.empty() || line[0] == '#') continue;

        // 计算缩进(2 空格 = 1 层级)
        int indent = 0;
        while (indent < static_cast<int>(line.size()) && line[indent] == ' ') {
            ++indent;
        }
        if (indent == static_cast<int>(line.size())) continue; // 纯空白行

        // 去除尾部回车
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string lineContent = line.substr(indent);
        // 去除首尾空格
        auto trimStart = lineContent.find_first_not_of(' ');
        if (trimStart != std::string::npos) {
            lineContent = lineContent.substr(trimStart);
        }

        // 调整前缀栈
        int indentLevel = indent / 2;
        while (static_cast<int>(prefixStack.size()) > indentLevel) {
            prefixStack.pop_back();
        }

        // ====================================================================
        // YAML 列表项处理: "- item" 语法 [v2.0.0]
        // ====================================================================
        if (lineContent.size() >= 2 && lineContent[0] == '-' && lineContent[1] == ' ') {
            std::string itemValue = lineContent.substr(2);
            auto itemTrim = itemValue.find_first_not_of(' ');
            if (itemTrim != std::string::npos) {
                itemValue = itemValue.substr(itemTrim);
            }
            // 去除尾部空格
            while (!itemValue.empty() && itemValue.back() == ' ') {
                itemValue.pop_back();
            }

            // 重置列表索引(当缩进层级变化时)
            if (static_cast<int>(prefixStack.size()) != indentLevel) {
                listIndex = 0;
            }

            // 构建完整键路径: prefix[0], prefix[1], ...
            std::string fullKey;
            for (size_t i = 0; i < prefixStack.size(); ++i) {
                if (i > 0) fullKey += '.';
                fullKey += prefixStack[i];
            }
            if (!fullKey.empty()) fullKey += '.';
            fullKey += "[" + std::to_string(listIndex) + "]";
            ++listIndex;

            // 存储值
            std::lock_guard<std::mutex> lock(m_mutex);
            storeValue(fullKey, itemValue);
            continue;
        }

        auto colonPos = lineContent.find(':');
        if (colonPos == std::string::npos) continue;

        listIndex = 0;  // 非列表行重置索引 [v2.0.0]

        std::string key = lineContent.substr(0, colonPos);
        while (!key.empty() && key.back() == ' ') key.pop_back();

        std::string value = lineContent.substr(colonPos + 1);
        // 去除 value 首尾空格
        auto vStart = value.find_first_not_of(' ');
        auto vEnd = value.find_last_not_of(' ');
        if (vStart != std::string::npos) {
            value = value.substr(vStart, vEnd - vStart + 1);
        } else {
            value.clear();
        }

        // 处理引号: 去除首尾匹配的引号对
        bool isQuoted = false;
        if (value.size() >= 2) {
            char first = value.front();
            char last = value.back();
            if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
                value = value.substr(1, value.size() - 2);
                isQuoted = true;
            }
        }

        prefixStack.push_back(key);

        // 构建完整键路径
        std::string fullKey;
        for (size_t i = 0; i < prefixStack.size(); ++i) {
            if (i > 0) fullKey += '.';
            fullKey += prefixStack[i];
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        // ====================================================================
        // YAML 内联列表处理: key: [a, b, c] 语法 [v2.0.0]
        // ====================================================================
        if (!isQuoted && !value.empty() && value.front() == '[' && value.back() == ']') {
            std::string inner = value.substr(1, value.size() - 2);
            std::istringstream innerStream(inner);
            std::string item;
            int idx = 0;
            while (std::getline(innerStream, item, ',')) {
                // 去除首尾空格
                auto itemStart = item.find_first_not_of(' ');
                auto itemEnd = item.find_last_not_of(' ');
                if (itemStart != std::string::npos) {
                    item = item.substr(itemStart, itemEnd - itemStart + 1);
                }
                // 去除引号
                if (item.size() >= 2 &&
                    ((item.front() == '"' && item.back() == '"') ||
                     (item.front() == '\'' && item.back() == '\''))) {
                    item = item.substr(1, item.size() - 2);
                }
                std::string itemKey = fullKey + "[" + std::to_string(idx) + "]";
                storeValue(itemKey, item);
                ++idx;
            }
            continue;
        }

        // 存储值
        if (value.empty()) {
            // 空值:可能是子节点标记,暂不存储
            m_values[fullKey] = QVariant(QString());
        } else if (isQuoted) {
            // 引号包裹的值始终作为字符串存储
            m_values[fullKey] = QVariant(QString::fromStdString(value));
        } else {
            storeValue(fullKey, value);
        }
    }
}

} // namespace sc