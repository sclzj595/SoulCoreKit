#ifndef SOUL_UTILS_FILE_FILE_UTILS_H
#define SOUL_UTILS_FILE_FILE_UTILS_H

#include <QString>
#include <QByteArray>
#include <vector>

namespace sc::utils::file {

bool exists(const QString& path);
bool isFile(const QString& path);
bool isDirectory(const QString& path);

bool createDirectory(const QString& path);
bool createDirectories(const QString& path);

bool remove(const QString& path);
bool removeRecursively(const QString& path);

bool rename(const QString& oldName, const QString& newName);
bool copy(const QString& source, const QString& destination);

QByteArray readFile(const QString& filePath);
bool writeFile(const QString& filePath, const QByteArray& content);

QString fileName(const QString& path);
QString baseName(const QString& path);
QString suffix(const QString& path);
QString directory(const QString& path);

QString joinPath(const QString& path1, const QString& path2);

std::vector<QString> listFiles(const QString& directory, const QString& pattern = "*");
std::vector<QString> listDirectories(const QString& directory);

qint64 fileSize(const QString& filePath);

}

#endif