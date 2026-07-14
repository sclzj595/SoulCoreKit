#include "soul/event/qt_signal_adapter.h"

namespace sc {

QtSignalAdapter::QtSignalAdapter(QObject* parent) : QObject(parent) {}

void QtSignalAdapter::connectSignalToMessageBus(QObject* sender, const char* signal, MessageBusPtr messageBus, const QString& channel) {
    m_messageBus = messageBus;
    m_currentChannel = channel;
    connect(sender, signal, this, SLOT(onSignalEmitted()));
}

void QtSignalAdapter::onSignalEmitted() {
    if (m_messageBus && !m_currentChannel.isEmpty()) {
        m_messageBus->publish(m_currentChannel, nullptr);
    }
}

}