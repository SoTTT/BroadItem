#include <broaditem/element/SizedElement.h>

namespace BroadItem {

/// @brief 从 XML 元素解析 width 和 height 属性。
/// @param xml The DOM element to parse.
void SizedElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    if (hasLiteralAttribute(xml, "width"))
        m_width = parseDouble(literalAttribute(xml, "width"), -1);
    if (hasLiteralAttribute(xml, "height"))
        m_height = parseDouble(literalAttribute(xml, "height"), -1);
}

void SizedElement::resolveSize(const LayoutContext& ctx, ResolvedStyle& out) const
{
    out.width = m_width;
    out.height = m_height;
    out.width = resolveDouble("width", ctx, out.width);
    out.height = resolveDouble("height", ctx, out.height);
}

} // namespace BroadItem
