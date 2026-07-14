#include "soul/utils/file/file_utils.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>

namespace sc::utils::file {

bool exists(const QString& path) {
    return QFile::exists(path);
}

bool isFile(const QString& path) {
    QFileInfo info(path);
    return info.isFile();
}

bool isDirectory(const QString& path) {
    QFileInfo info(path);
    return info.isDir();
}

bool createDirectory(const QString& path) {
    return QDir().mkdir(path);
}

bool createDirectories(const QString& path) {
    return QDir().mkpath(path);
}

bool remove(const QString& path) {
    return QFile::remove(path);
}

bool removeRecursively(const QString& path) {
    QDir dir(path);
    return dir.removeRecursively();
}

bool rename(const QString& oldName, const QString& newName) {
    return QFile::rename(oldName, newName);
}

bool copy(const QString& source, const QString& destination) {
    return QFile::copy(source, destination);
}

QByteArray readFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

bool writeFile(const QString& filePath, const QByteArray& content) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(content);
    return true;
}

QString fileName(const QString& path) {
    QFileInfo info(path);
    return info.fileName();
}

QString baseName(const QString& path) {
    QFileInfo info(path);
    return info.baseName();
}

QString suffix(const QString& path) {
    QFileInfo info(path);
    return info.suffix();
}

QString directory(const QString& path) {
    QFileInfo info(path);
    return info.path();
}

QString joinPath(const QString& path1, const QString& path2) {
    return QDir::cleanPath(path1 + "/" + path2);
}

std::vector<QString> listFiles(const QString& directory, const QString& pattern) {
    std::vector<QString> files;
    QDir dir(directory);
    QStringList entries = dir.entryList(QStringList(pattern), QDir::Files);
    for (const QString& entry : entries) {
        files.push_back(dir.absoluteFilePath(entry));
    }
    return files;
}

std::vector<QString> listDirectories(const QString& directory) {
    std::vector<QString> dirs;
    QDir dir(directory);
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        dirs.push_back(dir.absoluteFilePath(entry));
    }
    return dirs;
}

qint64 fileSize(const QString& filePath) {
    QFileInfo info(filePath);
    return info.size();
}

}