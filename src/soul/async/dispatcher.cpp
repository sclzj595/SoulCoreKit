#include "soul/async/dispatcher.h"
#include <functional>
#include <QTimer>
#include <QCoreApplication>
#include <QThreadPool>

namespace sc {

void Dispatcher::async(std::function<void()> task) {
    threadPool().start(task);
}

void Dispatcher::async(std::function<void()> task, int priority) {
    QThreadPool::globalInstance()->start(task, priority);
}

void Dispatcher::dispatchToMain(std::function<void()> task) {
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        task();
    } else {
        QMetaObject::invokeMethod(QCoreApplication::instance(), task, Qt::QueuedConnection);
    }
}

void Dispatcher::dispatchToMainDelayed(std::function<void()> task, int delayMs) {
    QTimer::singleShot(delayMs, QCoreApplication::instance(), task);
}

void Dispatcher::runOnWorker(std::function<void()> task) {
    threadPool().start(task);
}

void Dispatcher::runOnWorker(std::function<void()> task, int priority) {
    threadPool().start(task, priority);
}

void Dispatcher::setMaxThreads(int maxThreads) {
    threadPool().setMaxThreadCount(maxThreads);
}

int Dispatcher::maxThreads() {
    return threadPool().maxThreadCount();
}

QThreadPool& Dispatcher::threadPool() {
    return *QThreadPool::globalInstance();
}

}