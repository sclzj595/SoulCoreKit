#include "soul/utils/compress/compress_utils.h"
#include <QByteArray>

namespace sc {

QByteArray CompressUtils::gzipCompress(const QByteArray& data, int level) {
    Q_UNUSED(level);
    return qCompress(data, level);
}

QByteArray CompressUtils::gzipDecompress(const QByteArray& data) {
    return qUncompress(data);
}

QByteArray CompressUtils::zlibCompress(const QByteArray& data, int level) {
    Q_UNUSED(level);
    return qCompress(data, level);
}

QByteArray CompressUtils::zlibDecompress(const QByteArray& data) {
    return qUncompress(data);
}

bool CompressUtils::compressFile(const QString& sourcePath, const QString& destPath) {
    Q_UNUSED(sourcePath);
    Q_UNUSED(destPath);
    return false;
}

bool CompressUtils::decompressFile(const QString& sourcePath, const QString& destPath) {
    Q_UNUSED(sourcePath);
    Q_UNUSED(destPath);
    return false;
}

bool CompressUtils::isGzip(const QByteArray& data) {
    return data.size() >= 2 && static_cast<unsigned char>(data[0]) == 0x1f && static_cast<unsigned char>(data[1]) == 0x8b;
}

bool CompressUtils::isZlib(const QByteArray& data) {
    return data.size() >= 2 && (data[0] & 0x0f) == 8;
}

}