#include <broaditem/element/SizedElement.h>

namespace BroadItem {

/// @brief 返回有求值路径的绑定属性集合：width/height 并入 RenderableElement 的盒模型集合。
/// @return 静态引用（沿用 supportedAttributes 的 static-union 写法）。
const QSet<QString>& SizedElement::resolvedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{"width", "height"}
                                       + RenderableElement::resolvedAttributes();
    return attrs;
}

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
