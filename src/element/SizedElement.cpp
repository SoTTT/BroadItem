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
    // 保留原静默回退行为：非法/负值字面量视为未指定（不进入 Optional）。
    if (hasLiteralAttribute(xml, "width")) {
        const double w = parseDouble(literalAttribute(xml, "width"), -1);
        if (w >= 0)
            m_width = w;
    }
    if (hasLiteralAttribute(xml, "height")) {
        const double h = parseDouble(literalAttribute(xml, "height"), -1);
        if (h >= 0)
            m_height = h;
    }
}

void SizedElement::resolveSize(const LayoutContext& ctx, ResolvedStyle& out) const
{
    out.width = m_width;
    out.height = m_height;
    out.width = resolveDouble("width", ctx, out.width);
    out.height = resolveDouble("height", ctx, out.height);
    // 负值（非法字面量/绑定值）一律视为未指定，与 parse 侧过滤对齐
    if (out.width && *out.width < 0)
        out.width.reset();
    if (out.height && *out.height < 0)
        out.height.reset();
}

} // namespace BroadItem
