#include "soul/utils/process/process_utils.h"
#include <functional>
#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>

namespace sc {

bool ProcessUtils::execute(const QString& program, const QStringList& arguments,
                           QString* output, QString* error) {
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished()) {
        if (error) {
            *error = process.errorString();
        }
        return false;
    }
    if (output) {
        *output = process.readAllStandardOutput();
    }
    if (error) {
        *error = process.readAllStandardError();
    }
    return process.exitCode() == 0;
}

bool ProcessUtils::executeAsync(const QString& program, const QStringList& arguments,
                                std::function<void(int exitCode, const QString& output)> callback) {
    QProcess* process = new QProcess();
    if (callback) {
        QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         [process, callback](int exitCode, QProcess::ExitStatus) {
            QString output = process->readAllStandardOutput();
            callback(exitCode, output);
            process->deleteLater();
        });
    } else {
        QObject::connect(process, &QProcess::finished, process, &QProcess::deleteLater);
    }
    process->start(program, arguments);
    return true;
}

qint64 ProcessUtils::executeDetached(const QString& program, const QStringList& arguments) {
    qint64 pid = 0;
    QProcess::startDetached(program, arguments, QString(), &pid);
    return pid;
}

QString ProcessUtils::getCurrentProcessId() {
    return QString::number(QCoreApplication::applicationPid());
}

QString ProcessUtils::getCurrentProcessName() {
    return QCoreApplication::applicationName();
}

bool ProcessUtils::killProcess(qint64 pid) {
    QProcess process;
#ifdef Q_OS_WIN
    process.start("taskkill", QStringList() << "/F" << "/PID" << QString::number(pid));
#else
    process.start("kill", QStringList() << "-9" << QString::number(pid));
#endif
    return process.waitForFinished() && process.exitCode() == 0;
}

bool ProcessUtils::terminateProcess(qint64 pid) {
    QProcess process;
#ifdef Q_OS_WIN
    process.start("taskkill", QStringList() << "/PID" << QString::number(pid));
#else
    process.start("kill", QStringList() << QString::number(pid));
#endif
    return process.waitForFinished() && process.exitCode() == 0;
}

bool ProcessUtils::isProcessRunning(qint64 pid) {
#ifdef Q_OS_WIN
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << QString("PID eq %1").arg(pid));
    process.waitForFinished();
    return process.readAllStandardOutput().contains(QByteArray::number(pid));
#else
    QProcess process;
    process.start("ps", QStringList() << "-p" << QString::number(pid));
    process.waitForFinished();
    return process.exitCode() == 0;
#endif
}

QString ProcessUtils::findExecutable(const QString& name) {
    return QStandardPaths::findExecutable(name);
}

}