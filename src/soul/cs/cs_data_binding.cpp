// ============================================================================
// cs_data_binding.cpp — CS 数据绑定引擎实现 [v2.5.0]
// ============================================================================

#include "soul/cs/cs_data_binding.h"
#include "soul/cs/cs_view_model.h"

namespace sc::cs {

CsDataBinding::CsDataBinding(QObject* parent)
    : QObject(parent)
{
}

// syncToViewModel / syncToEntity 为模板方法，实现在 cs_data_binding.h 中。
// 原因: 模板方法需要访问 ReflectiveEntity 的 dynamic_cast，必须在头文件中实例化。

void CsDataBinding::registerConverter(const QString& propertyName,
                                       std::shared_ptr<ICsTypeConverter> converter) {
    if (converter) {
        m_converters[propertyName.toStdString()] = std::move(converter);
    }
}

void CsDataBinding::addMapping(const QString& entityProperty, const QString& viewModelProperty) {
    m_mappings[entityProperty.toStdString()] = viewModelProperty;
}

void CsDataBinding::clearMappings() {
    m_mappings.clear();
}

QVariant CsDataBinding::convertValue(const QString& propertyName,
                                      const QVariant& value,
                                      bool toViewModel) {
    auto it = m_converters.find(propertyName.toStdString());
    if (it != m_converters.end() && it->second) {
        return toViewModel ? it->second->toViewModel(value)
                           : it->second->toEntity(value);
    }
    return value;
}

} // namespace sc::cs
