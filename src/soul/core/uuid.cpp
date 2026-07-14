#include "soul/core/uuid.h"

#include <random>
#include <sstream>
#include <iomanip>

namespace sc {

std::string Uuid::generate() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);

    uint8_t data[16];
    for (int i = 0; i < 16; i++) {
        data[i] = static_cast<uint8_t>(dis(gen));
    }
    
    data[6] = (data[6] & 0x0F) | 0x40;
    data[8] = (data[8] & 0x3F) | 0x80;

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for (int i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) ss << "-";
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

bool Uuid::isValid(const std::string& uuid) {
    if (uuid.size() != 36) return false;
    for (int i = 0; i < 36; i++) {
        char c = uuid[i];
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != '-') return false;
        } else {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                return false;
            }
        }
    }
    return true;
}

std::string Uuid::format(const std::string& uuid) {
    std::string result = uuid;
    for (char& c : result) {
        if (c >= 'a' && c <= 'f') c -= 32;
    }
    return result;
}

}