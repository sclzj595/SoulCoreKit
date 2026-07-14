#include "soul/network/session.h"

namespace sc {

Session& Session::instance() {
    static Session inst;
    return inst;
}

QString Session::token() const {
    return m_token;
}

void Session::setToken(const QString& token) {
    m_token = token;
}

QString Session::get(const QString& key) const {
    return m_data.value(key);
}

void Session::set(const QString& key, const QString& value) {
    m_data[key] = value;
}

void Session::clear() {
    m_token.clear();
    m_data.clear();
}

bool Session::isEmpty() const {
    return m_token.isEmpty() && m_data.isEmpty();
}

}