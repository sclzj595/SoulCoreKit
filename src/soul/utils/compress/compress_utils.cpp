#include "soul/utils/compress/compress_utils.h"
#include <QtZlib/zlib.h>
#include <QByteArray>
#include <vector>

namespace sc {

namespace {

// 通用 deflate 压缩实现,通过 windowBits 区分 gzip/zlib 格式
// windowBits=15: zlib 格式; windowBits=15+16: gzip 格式; windowBits=15+32: 自动检测
QByteArray deflateCompress(const QByteArray& data, int level, int windowBits) {
    if (data.isEmpty()) {
        return QByteArray();
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    int ret = deflateInit2(&strm, level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        return QByteArray();
    }

    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.constData()));
    strm.avail_in = static_cast<uInt>(data.size());

    // 预留空间:输入大小 + 1024 字节(头部/尾部开销)
    QByteArray output;
    output.resize(data.size() + 1024);

    strm.next_out = reinterpret_cast<Bytef*>(output.data());
    strm.avail_out = static_cast<uInt>(output.size());

    ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        return QByteArray();
    }

    output.resize(static_cast<int>(strm.total_out));
    deflateEnd(&strm);
    return output;
}

// 通用 inflate 解压实现,通过 windowBits 区分 gzip/zlib 格式
QByteArray inflateDecompress(const QByteArray& data, int windowBits) {
    if (data.isEmpty()) {
        return QByteArray();
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.constData()));
    strm.avail_in = static_cast<uInt>(data.size());

    int ret = inflateInit2(&strm, windowBits);
    if (ret != Z_OK) {
        return QByteArray();
    }

    std::vector<char> buffer;
    buffer.resize(8192);

    QByteArray output;
    output.reserve(data.size() * 4);

    do {
        strm.next_out = reinterpret_cast<Bytef*>(buffer.data());
        strm.avail_out = static_cast<uInt>(buffer.size());

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            return QByteArray();
        }

        output.append(buffer.data(), static_cast<int>(buffer.size() - strm.avail_out));
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return output;
}

} // anonymous namespace

QByteArray CompressUtils::gzipCompress(const QByteArray& data, int level) {
    // windowBits = 15 + 16 → gzip 格式
    return deflateCompress(data, level, 15 + 16);
}

QByteArray CompressUtils::gzipDecompress(const QByteArray& data) {
    // windowBits = 15 + 16 → gzip 格式
    return inflateDecompress(data, 15 + 16);
}

QByteArray CompressUtils::zlibCompress(const QByteArray& data, int level) {
    // windowBits = 15 → zlib 格式
    return deflateCompress(data, level, 15);
}

QByteArray CompressUtils::zlibDecompress(const QByteArray& data) {
    // windowBits = 15 → zlib 格式
    return inflateDecompress(data, 15);
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
    // gzip 魔数: 0x1f 0x8b
    return data.size() >= 2 &&
           static_cast<unsigned char>(data[0]) == 0x1f &&
           static_cast<unsigned char>(data[1]) == 0x8b;
}

bool CompressUtils::isZlib(const QByteArray& data) {
    // zlib 头检测: CMF 字节低 4 位(CM)必须为 8(deflate),
    // 高 4 位(CINFO)通常为 7(32K 窗口);且 (CMF*256 + FLG) % 31 == 0
    if (data.size() < 2) {
        return false;
    }
    unsigned char cmf = static_cast<unsigned char>(data[0]);
    unsigned char flg = static_cast<unsigned char>(data[1]);
    if ((cmf & 0x0f) != 8) {
        return false;
    }
    if (((cmf << 8) | flg) % 31 != 0) {
        return false;
    }
    return true;
}

} // namespace sc
