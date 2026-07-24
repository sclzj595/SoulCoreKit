#ifndef SOUL_UTILS_PROCESS_UTILS_H
#define SOUL_UTILS_PROCESS_UTILS_H

#include <QString>
#include <QProcess>

namespace sc {

class ProcessUtils {
public:
    static bool execute(const QString& program, const QStringList& arguments,
                        QString* output = nullptr, QString* error = nullptr);

    static bool executeAsync(const QString& program, const QStringList& arguments,
                             std::function<void(int exitCode, const QString& output)> callback = nullptr);

    static qint64 executeDetached(const QString& program, const QStringList& arguments);

    static QString getCurrentProcessId();
    static QString getCurrentProcessName();

    static bool killProcess(qint64 pid);
    static bool terminateProcess(qint64 pid);

    static bool isProcessRunning(qint64 pid);

    static QString findExecutable(const QString& name);
};

}

#endif