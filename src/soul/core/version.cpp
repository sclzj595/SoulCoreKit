#include "soul/core/version.h"

#include <sstream>
#include <regex>
#include <string>

namespace sc {

bool Version::operator<(const Version& other) const {
    if (m_major != other.m_major) return m_major < other.m_major;
    if (m_minor != other.m_minor) return m_minor < other.m_minor;
    if (m_patch != other.m_patch) return m_patch < other.m_patch;
    return m_pre < other.m_pre;
}

bool Version::operator<=(const Version& other) const {
    return *this < other || *this == other;
}

bool Version::operator>(const Version& other) const {
    return !(*this <= other);
}

bool Version::operator>=(const Version& other) const {
    return !(*this < other);
}

bool Version::operator==(const Version& other) const {
    return m_major == other.m_major && m_minor == other.m_minor &&
           m_patch == other.m_patch && m_pre == other.m_pre;
}

bool Version::operator!=(const Version& other) const {
    return !(*this == other);
}

std::string Version::toString() const {
    std::stringstream ss;
    ss << m_major << "." << m_minor << "." << m_patch;
    if (!m_pre.empty()) ss << "-" << m_pre;
    if (!m_build.empty()) ss << "+" << m_build;
    return ss.str();
}

Version Version::parse(const std::string& versionStr) {
    std::regex pattern(R"(^(\d+)\.(\d+)\.(\d+)(?:-([a-zA-Z0-9.-]+))?(?:\+([a-zA-Z0-9.-]+))?$)");
    std::smatch match;
    if (std::regex_match(versionStr, match, pattern)) {
        return Version(
            std::stoi(match[1]),
            std::stoi(match[2]),
            std::stoi(match[3]),
            match[4].str(),
            match[5].str()
        );
    }
    return Version();
}

}