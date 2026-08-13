// ============================================================================
// cs_service.cpp — CS 服务层基类实现 [v2.1.0]
// ============================================================================

#include "soul/cs/cs_service.h"

namespace sc::cs {

CsService::CsService(const QString& serviceName, QObject* parent)
    : QObject(parent)
    , m_serviceName(serviceName)
    , m_serviceVersion("1.0.0")
{
}

} // namespace sc::cs
