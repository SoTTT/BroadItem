#include <broaditem/context/ItemPropertyContext.h>

namespace BroadItem {

/// @brief 构造链式属性上下文。
/// @param itemContext 当前迭代项的属性上下文（可为 nullptr）。
/// @param globalContext 全局属性上下文（可为 nullptr）。
/// @param asVariable `<for as="xxx">` 中的迭代变量名。
ItemPropertyContext::ItemPropertyContext(MapPropertyContext* itemContext,
                                         PropertyContext* globalContext,
                                         const QString& asVariable)
    : m_itemContext(itemContext)
    , m_globalContext(globalContext)
    , m_asVariable(asVariable)
{
}

/// @brief 剥离 asVariable 前缀。若不以 "asVariable." 开头则原样返回。
/// @param name 要处理的属性名。
/// @return 剥离前缀后的属性名。
QString ItemPropertyContext::stripAsPrefix(const QString& name) const
{
    QString prefix = m_asVariable + ".";
    if (name.startsWith(prefix))
        return name.mid(prefix.length());
    return name;
}

/// @brief 解析属性值，优先从 itemContext 查找，未命中则 fallback 到 globalContext。
/// 支持 asVariable 前缀剥离。
/// @param name 属性名。
/// @return 属性值，未找到时返回无效 QVariant。
QVariant ItemPropertyContext::property(const QString& name) const
{
    QString stripped = stripAsPrefix(name);
    bool hasPrefix = (stripped != name);

    // 1. 名称以 "asVariable." 开头：剥离前缀后在迭代项上下文中查找。
    //    MapPropertyContext::property() 内部走 resolveFirstThenWalk，
    //    嵌套路径遍历由 PropertyContext::walkNested 完成。
    if (hasPrefix && m_itemContext) {
        QVariant v = m_itemContext->property(stripped);
        if (v.isValid())
            return v;
    }

    // 2. 无前缀：在迭代项上下文中精确匹配（扁平键或点号路径）。
    if (!hasPrefix && m_itemContext) {
        QVariant v = m_itemContext->property(name);
        if (v.isValid())
            return v;
    }

    // 3. 回退到全局上下文。
    if (m_globalContext)
        return m_globalContext->property(name);

    return QVariant();
}

/// @brief 检查属性是否存在，优先检查 itemContext，再 fallback 到 globalContext。
/// @param name 属性名。
/// @return 如果属性存在则返回 true。
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

/// @brief 设置属性值，直接委托到 globalContext（itemContext 为只读）。
/// @param name 属性名（扁平键，不支持路径）。
/// @param value 属性值。
void ItemPropertyContext::setProperty(const QString& name, const QVariant& value)
{
    if (m_globalContext)
        m_globalContext->setProperty(name, value);
}

} // namespace BroadItem
