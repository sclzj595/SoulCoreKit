#include "soul/async/async_runner.h"
#include <QMetaObject>
#include <QCoreApplication>
#include <QTimer>

namespace sc {

void AsyncRunner::runAsync(const Task& task) {
    QThread::create(task)->start();
}

void AsyncRunner::runAsync(const Task& task, const ResultCallback& callback) {
    QThread::create([task, callback]() {
        try {
            task();
            runOnMainThread([callback]() { callback(true); });
        } catch (...) {
            runOnMainThread([callback]() { callback(false); });
        }
    })->start();
}

void AsyncRunner::runOnMainThread(const Task& task) {
    QMetaObject::invokeMethod(QCoreApplication::instance(), task, Qt::QueuedConnection);
}

void AsyncRunner::runDelayed(const Task& task, int milliseconds) {
    QTimer::singleShot(milliseconds, task);
}

}