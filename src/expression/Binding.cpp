#include <broaditem/expression/Binding.h>

namespace BroadItem {

/// @brief 用属性名和路径字符串构造绑定。
/// @param attributeName XML 属性名（如 ":content"）。
/// @param path 数据绑定路径（如 "user.name"）。
Binding::Binding(const QString& attributeName, const QString& path)
    : m_attributeName(attributeName)
    , m_expression(path)
{
}

/// @brief 检查此绑定的路径是否匹配给定属性名。
/// 使用与 Element::matchesProperty() 相同的前缀匹配语义。
/// @param propName 要检查的属性名。
/// @return 如果绑定路径匹配该属性则返回 true。
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
