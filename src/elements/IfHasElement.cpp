#include "broaditem/elements/IfHasElement.h"
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

void IfHasElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    if (xml.hasAttribute(":prop"))
        m_propertyName = xml.attribute(":prop");
    m_not = xml.hasAttribute("not");
}

void IfHasElement::setBindProperty(const QString& bind)
{
    m_propertyName = bind;
}

void IfHasElement::setNot(bool notValue)
{
    m_not = notValue;
}

void IfHasElement::setChild(ElementPtr child)
{
    m_child = std::move(child);
}

ElementPtr IfHasElement::clone() const
{
    auto copy = std::make_shared<IfHasElement>();
    copy->m_propertyName = m_propertyName;
    copy->m_not = m_not;
    if (m_child)
        copy->m_child = m_child->clone();
    return copy;
}

bool IfHasElement::shouldShow(const LayoutContext& ctx) const
{
    bool has = ctx.hasProperty(m_propertyName);
    return m_not ? !has : has;
}

std::vector<ElementPtr> IfHasElement::expand(const LayoutContext& ctx) const
{
    if (!shouldShow(ctx) || !m_child)
        return {};
    return { m_child->clone() };
}

MeasureResult IfHasElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    if (!shouldShow(ctx) || !m_child)
        return MeasureResult{Size{0, 0}};
    return m_child->measure(ctx, constraints);
}

void IfHasElement::layout(const LayoutContext& ctx, const Rect& rect)
{
    m_rect = rect;
    if (!shouldShow(ctx) || !m_child)
        return;
    m_child->layout(ctx, rect);
}

void IfHasElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    if (!shouldShow(ctx) || !m_child)
        return;
    m_child->render(painter, ctx);
}

bool IfHasElement::bindsProperty(const QString& name) const
{
    return matchesProperty(m_propertyName, name) || (m_child && m_child->bindsProperty(name));
}

} // namespace BroadItem
