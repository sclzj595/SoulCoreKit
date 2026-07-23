#ifndef SOUL_EVENT_QT_SIGNAL_ADAPTER_H
#define SOUL_EVENT_QT_SIGNAL_ADAPTER_H

#include <QObject>
#include <QMetaMethod>
#include <memory>
#include "soul/event/i_message_bus.h"

namespace sc {

class QtSignalAdapter : public QObject {
    Q_OBJECT
public:
    explicit QtSignalAdapter(QObject* parent = nullptr);
    ~QtSignalAdapter() override = default;

    void connectSignalToMessageBus(QObject* sender, const char* signal, MessageBusPtr messageBus, const QString& channel);

private slots:
    void onSignalEmitted();

private:
    QObject* m_currentSender;
    MessageBusPtr m_messageBus;
    QString m_currentChannel;
};

}

#endif