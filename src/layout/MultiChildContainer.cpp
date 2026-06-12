#include <broaditem/layout/MultiChildContainer.h>
#include <broaditem/control/ForElement.h>
#include <broaditem/control/IfHasElement.h>
#include <QPainter>
#include <algorithm>

namespace BroadItem {

void MultiChildContainer::addChild(ElementPtr child)
{
    m_children.push_back(std::move(child));
}

void MultiChildContainer::resolveBindings(const LayoutContext& ctx)
{
    ContainerElement::resolveBindings(ctx);
    for (const auto& child : m_children) {
        if (child)
            child->resolveBindings(ctx);
    }
}

std::vector<ElementPtr> MultiChildContainer::flattenChildren(const LayoutContext& ctx) const
{
    std::vector<ElementPtr> flat;
    for (const auto& child : m_children) {
        if (!child)
            continue;
        if (auto forEl = std::dynamic_pointer_cast<ForElement>(child)) {
            auto expanded = forEl->expand(ctx);
            flat.insert(flat.end(), expanded.begin(), expanded.end());
        } else if (auto ifEl = std::dynamic_pointer_cast<IfHasElement>(child)) {
            auto expanded = ifEl->expand(ctx);
            flat.insert(flat.end(), expanded.begin(), expanded.end());
        } else {
            flat.push_back(child);
        }
    }
    return flat;
}

void MultiChildContainer::render(QPainter* painter, const LayoutContext& ctx) const
{
    ContainerElement::render(painter, ctx);
    for (const auto& child : m_flattened) {
        child->render(painter, ctx);
    }
}

bool MultiChildContainer::bindsProperty(const QString& name) const
{
    if (ContainerElement::bindsProperty(name))
        return true;
    return std::any_of(m_children.begin(), m_children.end(),
        [&](const auto& child) { return child && child->bindsProperty(name); });
}

} // namespace BroadItem
