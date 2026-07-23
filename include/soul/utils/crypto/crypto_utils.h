#ifndef SOUL_UTILS_CRYPTO_CRYPTO_UTILS_H
#define SOUL_UTILS_CRYPTO_CRYPTO_UTILS_H

#include <QByteArray>
#include <QString>

namespace sc::utils::crypto {

QByteArray md5(const QByteArray& data);
QByteArray md5File(const QString& filePath);
QString md5Hex(const QByteArray& data);

QByteArray sha1(const QByteArray& data);
QString sha1Hex(const QByteArray& data);

QByteArray sha256(const QByteArray& data);
QString sha256Hex(const QByteArray& data);

QByteArray hmacSha256(const QByteArray& data, const QByteArray& key);

QByteArray encryptAes(const QByteArray& data, const QByteArray& key);
QByteArray decryptAes(const QByteArray& data, const QByteArray& key);

QByteArray encryptAesCbc(const QByteArray& data, const QByteArray& key, const QByteArray& iv);
QByteArray decryptAesCbc(const QByteArray& data, const QByteArray& key, const QByteArray& iv);

QString base64Encode(const QByteArray& data);
QByteArray base64Decode(const QString& encoded);

QString urlEncode(const QString& str);
QString urlDecode(const QString& str);

QByteArray generateRandomBytes(int length);
QString generateRandomString(int length);

}

#endif