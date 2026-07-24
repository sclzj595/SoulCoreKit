#ifndef SOUL_UTILS_COMPRESS_COMPRESS_UTILS_H
#define SOUL_UTILS_COMPRESS_COMPRESS_UTILS_H

#include <QByteArray>
#include <QString>

namespace sc {

class CompressUtils {
public:
    static QByteArray gzipCompress(const QByteArray& data, int level = -1);
    static QByteArray gzipDecompress(const QByteArray& data);

    static QByteArray zlibCompress(const QByteArray& data, int level = -1);
    static QByteArray zlibDecompress(const QByteArray& data);

    static bool compressFile(const QString& sourcePath, const QString& destPath);
    static bool decompressFile(const QString& sourcePath, const QString& destPath);

    static bool isGzip(const QByteArray& data);
    static bool isZlib(const QByteArray& data);
};

}

#endif