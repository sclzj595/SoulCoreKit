#ifndef SOUL_STORAGE_JSON_SERIALIZER_H
#define SOUL_STORAGE_JSON_SERIALIZER_H

#include "i_serializer.h"
#include "soul/utils/json/json_helper.h"

namespace sc {

class JsonSerializer : public ISerializer {
public:
    QString name() const override;

    Result<QByteArray> serialize(const QVariant& data) const override;
    Result<QVariant> deserialize(const QByteArray& data) const override;

    void setCompact(bool compact) { m_compact = compact; }
    bool isCompact() const { return m_compact; }

private:
    bool m_compact = true;
};

}

#endif
