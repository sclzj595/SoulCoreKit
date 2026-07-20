#include "soul/utils/crypto/crypto_utils.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QFile>
#include <QRandomGenerator>
#include <QUrl>

namespace sc {
namespace utils {
namespace crypto {

QByteArray md5(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
}

QByteArray md5File(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(&file);
    return hash.result();
}

QString md5Hex(const QByteArray& data) {
    return md5(data).toHex();
}


QByteArray sha1(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha1);
}

QString sha1Hex(const QByteArray& data) {
    return sha1(data).toHex();
}

QByteArray sha256(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QString sha256Hex(const QByteArray& data) {
    return sha256(data).toHex();
}

QByteArray hmacSha256(const QByteArray& data, const QByteArray& key) {
    QMessageAuthenticationCode mac(QCryptographicHash::Sha256);
    mac.setKey(key);
    mac.addData(data);
    return mac.result();
}

QByteArray encryptAes(const QByteArray& data, const QByteArray& key) {
    Q_UNUSED(data);
    Q_UNUSED(key);
    return QByteArray();
}

QByteArray decryptAes(const QByteArray& data, const QByteArray& key) {
    Q_UNUSED(data);
    Q_UNUSED(key);
    return QByteArray();
}

QByteArray encryptAesCbc(const QByteArray& data, const QByteArray& key, const QByteArray& iv) {
    Q_UNUSED(data);
    Q_UNUSED(key);
    Q_UNUSED(iv);
    return QByteArray();
}

QByteArray decryptAesCbc(const QByteArray& data, const QByteArray& key, const QByteArray& iv) {
    Q_UNUSED(data);
    Q_UNUSED(key);
    Q_UNUSED(iv);
    return QByteArray();
}

QString base64Encode(const QByteArray& data) {
    return data.toBase64();
}

QByteArray base64Decode(const QString& encoded) {
    return QByteArray::fromBase64(encoded.toUtf8());
}

QString urlEncode(const QString& str) {
    return QString::fromUtf8(QUrl::toPercentEncoding(str));
}

QString urlDecode(const QString& str) {
    return QUrl::fromPercentEncoding(str.toUtf8());
}

QByteArray generateRandomBytes(int length) {
    QByteArray bytes;
    bytes.resize(length);
    QRandomGenerator::global()->fillRange(reinterpret_cast<uint32_t*>(bytes.data()), length / 4 + 1);
    return bytes;
}

QString generateRandomString(int length) {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString result;
    result.reserve(length);
    for (int i = 0; i < length; i++) {
        result += chars.at(QRandomGenerator::global()->bounded(chars.size()));
    }
    return result;
}

} // namespace crypto
} // namespace utils
} // namespace sc