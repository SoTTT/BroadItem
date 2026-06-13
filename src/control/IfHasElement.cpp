#include <broaditem/control/IfHasElement.h>
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 IfHasElement 支持的 XML 属性集合。
/// @return Reference to a static set containing "not" (b:prop is namespace-aware, not listed here).
const QSet<QString>& IfHasElement::supportedAttributes() const
{
    static const QSet<QString> attrs = {"not"};
    return attrs;
}

/// @brief 从 XML 元素解析 b:prop 和 not 属性。
/// @param xml The DOM element to parse.
void IfHasElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttributeNS(BINDING_NS, "prop")) {
        /// 获取实际的 XML 限定属性名（如 b:prop 或用户自定义前缀）
        QDomNode attrNode = xml.attributes().namedItemNS(BINDING_NS, "prop");
        QString attrName = attrNode.isNull() ? "b:prop" : attrNode.nodeName();
        m_binding = Binding(attrName, xml.attributeNS(BINDING_NS, "prop", QString()));
    }
    m_not = xml.hasAttribute("not");
}

/// @brief 设置要检查条件显示的属性名。
/// @param bind The property name.
void IfHasElement::setBindProperty(const QString& bind)
{
    m_binding = Binding("b:prop", bind);
}

/// @brief 设置条件是否取反。
/// @param notValue If true, the element is shown when the property is absent.
void IfHasElement::setNot(bool notValue)
{
    m_not = notValue;
}

/// @brief 设置要条件显示的子元素。
/// @param child The child element.
void IfHasElement::setChild(ElementPtr child)
{
    m_child = std::move(child);
}

/// @brief 创建此 IfHasElement 的深拷贝，包括子元素。
/// @return A new IfHasElement with cloned child.
ElementPtr IfHasElement::clone() const
{
    auto copy = std::make_shared<IfHasElement>();
    copy->m_binding = m_binding;
    copy->m_not = m_not;
    if (m_child)
        copy->m_child = m_child->clone();
    return copy;
}

/// @brief 根据属性存在性和空值检查决定是否显示子元素。
/// @param ctx The layout context to query.
/// @return True if the condition is met (respecting the "not" flag).
bool IfHasElement::shouldShow(const LayoutContext& ctx) const
{
    bool has = ctx.hasProperty(m_binding.path());
    if (has) {
        QVariant v = ctx.property(m_binding.path());
        has = !v.isNull();
    }
    return m_not ? !has : has;
}

/// @brief 条件满足时展开为克隆的子元素，否则返回空向量。
/// @param ctx The layout context.
/// @return A vector with one cloned child, or empty.
std::vector<ElementPtr> IfHasElement::expand(const LayoutContext& ctx) const
{
    if (!shouldShow(ctx) || !m_child)
        return {};
    auto instance = m_child->clone();
    instance->resolveBindings(ctx);
    return { std::move(instance) };
}

/// @brief 条件满足时测量子元素，否则返回零尺寸。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @return The child's measured size, or zero if hidden.
MeasureResult IfHasElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    if (!shouldShow(ctx) || !m_child)
        return MeasureResult{QSizeF(0, 0)};
    return m_child->measure(ctx, constraints);
}

/// @brief 条件满足时布局子元素。
/// @param ctx The layout context.
/// @param rect The bounding rectangle for the child.
void IfHasElement::layout(const LayoutContext& ctx, const QRectF& rect)
{
    m_rect = rect;
    if (!shouldShow(ctx) || !m_child)
        return;
    m_child->layout(ctx, rect);
}

/// @brief 条件满足时渲染子元素。
/// @param painter The QPainter to render onto.
/// @param ctx The layout context.
void IfHasElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    if (!shouldShow(ctx) || !m_child)
        return;
    m_child->render(painter, ctx);
}

/// @brief 检查此元素是否绑定指定属性（通过 b:prop 或在子元素中）。
/// @param name The property name to check.
/// @return True if the property is bound.
bool IfHasElement::bindsProperty(const QString& name) const
{
    return m_binding.bindsProperty(name) || (m_child && m_child->bindsProperty(name));
}

} // namespace BroadItem
