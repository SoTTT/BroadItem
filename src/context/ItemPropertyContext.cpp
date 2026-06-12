#include <broaditem/context/ItemPropertyContext.h>

namespace BroadItem {

ItemPropertyContext::ItemPropertyContext(MapPropertyContext* itemContext,
                                         PropertyContext* globalContext,
                                         const QString& asVariable)
    : m_itemContext(itemContext)
    , m_globalContext(globalContext)
    , m_asVariable(asVariable)
{
}

QString ItemPropertyContext::stripAsPrefix(const QString& name) const
{
    QString prefix = m_asVariable + ".";
    if (name.startsWith(prefix))
        return name.mid(prefix.length());
    return name;
}

QVariant ItemPropertyContext::property(const QString& name) const
{
    QString stripped = stripAsPrefix(name);
    bool hasPrefix = (stripped != name);

    // 1. If name starts with "asVariable.", strip prefix and look up in item context.
    //    MapPropertyContext::property() internally uses resolveFirstThenWalk
    //    which invokes PropertyContext::walkNested for nested path traversal.
    if (hasPrefix && m_itemContext) {
        QVariant v = m_itemContext->property(stripped);
        if (v.isValid())
            return v;
    }

    // 2. Without prefix, check item context for exact match (flat or dotted).
    if (!hasPrefix && m_itemContext) {
        QVariant v = m_itemContext->property(name);
        if (v.isValid())
            return v;
    }

    // 3. Fallback to global context.
    if (m_globalContext)
        return m_globalContext->property(name);

    return QVariant();
}

bool ItemPropertyContext::hasProperty(const QString& name) const
{
    QString stripped = stripAsPrefix(name);
    bool hasPrefix = (stripped != name);

    if (hasPrefix && m_itemContext) {
        if (m_itemContext->hasProperty(stripped))
            return true;
    }

    if (!hasPrefix && m_itemContext) {
        if (m_itemContext->hasProperty(name))
            return true;
    }

    if (m_globalContext)
        return m_globalContext->hasProperty(name);

    return false;
}

void ItemPropertyContext::setProperty(const QString& name, const QVariant& value)
{
    if (m_globalContext)
        m_globalContext->setProperty(name, value);
}

} // namespace BroadItem
