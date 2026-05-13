#include "broaditem/SizedElement.h"

namespace BroadItem {

void SizedElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    if (xml.hasAttribute("width"))
        m_width = parseDouble(xml.attribute("width"), -1);
    if (xml.hasAttribute("height"))
        m_height = parseDouble(xml.attribute("height"), -1);
}

} // namespace BroadItem
