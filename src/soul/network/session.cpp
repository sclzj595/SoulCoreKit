#include "soul/network/session.h"

namespace sc {
namespace network {

Session& Session::instance() {
    static Session inst;
    return inst;
}

QString Session::token() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_token;
}

void Session::setToken(const QString& token) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_token = token;
}

QString Session::get(const QString& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data.value(key);
}

void Session::set(const QString& key, const QString& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data[key] = value;
}

void Session::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_token.clear();
    m_data.clear();
}

bool Session::isEmpty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_token.isEmpty() && m_data.isEmpty();
}

} // namespace network
} // namespace sc