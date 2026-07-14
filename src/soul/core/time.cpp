#include "soul/core/time.h"

#include <iostream>

namespace sc {

Time::Timestamp Time::now() {
    return std::chrono::duration_cast<Timestamp>(std::chrono::system_clock::now().time_since_epoch());
}

std::string Time::nowString(const std::string& fmt) {
    return Time::format(now(), fmt);
}

std::string Time::format(Timestamp timestamp, const std::string& fmt) {
    auto tp = std::chrono::system_clock::time_point(timestamp);
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&tt);
    
    std::string result;
    size_t i = 0;
    while (i < fmt.size()) {
        if (fmt[i] == 'y' && fmt.substr(i, 4) == "yyyy") {
            char buf[5];
            std::strftime(buf, sizeof(buf), "%Y", &tm);
            result += buf;
            i += 4;
        } else if (fmt[i] == 'M' && fmt.substr(i, 2) == "MM") {
            char buf[3];
            std::strftime(buf, sizeof(buf), "%m", &tm);
            result += buf;
            i += 2;
        } else if (fmt[i] == 'd' && fmt.substr(i, 2) == "dd") {
            char buf[3];
            std::strftime(buf, sizeof(buf), "%d", &tm);
            result += buf;
            i += 2;
        } else if (fmt[i] == 'H' && fmt.substr(i, 2) == "HH") {
            char buf[3];
            std::strftime(buf, sizeof(buf), "%H", &tm);
            result += buf;
            i += 2;
        } else if (fmt[i] == 'm' && fmt.substr(i, 2) == "mm") {
            char buf[3];
            std::strftime(buf, sizeof(buf), "%M", &tm);
            result += buf;
            i += 2;
        } else if (fmt[i] == 's' && fmt.substr(i, 2) == "ss") {
            char buf[3];
            std::strftime(buf, sizeof(buf), "%S", &tm);
            result += buf;
            i += 2;
        } else if (fmt[i] == 'z' && fmt.substr(i, 3) == "zzz") {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp) % 1000;
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%03d", static_cast<int>(ms.count()));
            result += buf;
            i += 3;
        } else {
            result += fmt[i];
            i++;
        }
    }
    return result;
}

Time::Timestamp Time::parse(const std::string& timeStr, const std::string& format) {
    (void)timeStr;
    (void)format;
    return Timestamp(0);
}

std::string Time::toLocalTime(Timestamp timestamp) {
    return format(timestamp, "yyyy-MM-dd HH:mm:ss");
}

std::string Time::toUtcTime(Timestamp timestamp) {
    auto tp = std::chrono::system_clock::time_point(timestamp);
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::gmtime(&tt);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

Time::Timestamp Time::fromSeconds(int64_t seconds) {
    return Timestamp(seconds * 1000);
}

Time::Timestamp Time::fromMilliseconds(int64_t ms) {
    return Timestamp(ms);
}

int64_t Time::toSeconds(Timestamp timestamp) {
    return timestamp.count() / 1000;
}

int64_t Time::toMilliseconds(Timestamp timestamp) {
    return timestamp.count();
}

}