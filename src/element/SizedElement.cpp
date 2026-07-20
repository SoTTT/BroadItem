#include <broaditem/element/SizedElement.h>

namespace BroadItem {

/// @brief 从 XML 元素解析 width 和 height 属性。
/// @param xml The DOM element to parse.
void SizedElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    if (xml.hasAttribute("width"))
        m_width = parseDouble(xml.attribute("width"), -1);
    if (xml.hasAttribute("height"))
        m_height = parseDouble(xml.attribute("height"), -1);
}

void SizedElement::resolveSize(const LayoutContext& ctx, ResolvedStyle& out) const
{
    out.width = m_width;
    out.height = m_height;
    out.width = resolveDouble("width", ctx, out.width);
    out.height = resolveDouble("height", ctx, out.height);
}

} // namespace BroadItem
