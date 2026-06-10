#include "broaditem/Binding.h"

namespace BroadItem {

Binding::Binding(const QString& attributeName, const QString& path)
    : m_attributeName(attributeName)
    , m_expression(path)
{
}

bool Binding::bindsProperty(const QString& propName) const
{
    if (!m_expression.isValid())
        return false;

    const QString& bindPath = path();
    return bindPath == propName
        || bindPath.startsWith(propName + QStringLiteral("."))
        || bindPath.startsWith(propName + QStringLiteral("["));
}

} // namespace BroadItem
