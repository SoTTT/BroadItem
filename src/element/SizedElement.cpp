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

} // namespace BroadItem
