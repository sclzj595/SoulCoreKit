#ifndef SOUL_STORAGE_JSON_SERIALIZER_H
#define SOUL_STORAGE_JSON_SERIALIZER_H

#include "i_serializer.h"
#include <QJsonDocument>

namespace sc {

class JsonSerializer : public ISerializer {
public:
    QString name() const override;

    Result<QByteArray> serialize(const QVariant& data) const override;
    Result<QVariant> deserialize(const QByteArray& data) const override;

private:
    QJsonDocument::JsonFormat m_format = QJsonDocument::Compact;
};

}

#endif
