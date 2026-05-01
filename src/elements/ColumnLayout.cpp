#include "broaditem/elements/ColumnLayout.h"
#include "broaditem/elements/ForElement.h"
#include "broaditem/elements/IfHasElement.h"
#include <QPainter>
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

void ColumnLayout::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    if (xml.hasAttribute("main-align"))
        m_mainAlign = xml.attribute("main-align");
    if (xml.hasAttribute("cross-align"))
        m_crossAlign = xml.attribute("cross-align");
    if (xml.hasAttribute("space"))
        m_space = parseDouble(xml.attribute("space"));
}

void ColumnLayout::addChild(ElementPtr child)
{
    m_children.push_back(child);
}

ElementPtr ColumnLayout::clone() const
{
    auto copy = std::make_shared<ColumnLayout>();
    copy->decorators = decorators;
    copy->m_rect = m_rect;
    copy->m_mainAlign = m_mainAlign;
    copy->m_crossAlign = m_crossAlign;
    copy->m_space = m_space;
    for (const auto& child : m_children) {
        if (child)
            copy->m_children.push_back(child->clone());
    }
    return copy;
}

void ColumnLayout::interpolateValues(const QStringList& values)
{
    for (const auto& child : m_children) {
        if (child)
            child->interpolateValues(values);
    }
}

std::vector<ElementPtr> ColumnLayout::flattenChildren(const LayoutContext& ctx) const
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

MeasureResult ColumnLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    double totalHeight = 0;
    double maxWidth = 0;

    LayoutConstraints childConstraints = constraints;
    double decoH = decorators.totalHeight();
    double decoW = decorators.totalWidth();

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    m_flattened = flattenChildren(ctx);
    for (size_t i = 0; i < m_flattened.size(); ++i) {
        auto result = m_flattened[i]->measure(ctx, childConstraints);
        totalHeight += result.intrinsicSize.height;
        maxWidth = std::max(maxWidth, result.intrinsicSize.width);
        if (i + 1 < m_flattened.size())
            totalHeight += m_space;
    }

    Size sz;
    sz.width = maxWidth + decoW;
    sz.height = totalHeight + decoH;
    return MeasureResult{sz};
}

void ColumnLayout::layout(const LayoutContext& ctx, const Rect& rect)
{
    ContainerElement::layout(ctx, rect);

    // Content rect after decorators
    Rect contentRect;
    contentRect.pos.x = rect.pos.x + decorators.margin.left + decorators.border.width + decorators.padding.left;
    contentRect.pos.y = rect.pos.y + decorators.margin.top + decorators.border.width + decorators.padding.top;
    contentRect.size.width = std::max(0.0, rect.size.width - decorators.totalWidth());
    contentRect.size.height = std::max(0.0, rect.size.height - decorators.totalHeight());

    m_flattened = flattenChildren(ctx);
    layoutChildren(ctx, contentRect);
}

void ColumnLayout::layoutChildren(const LayoutContext& ctx, const Rect& contentRect)
{
    if (m_flattened.empty())
        return;

    // Measure all children
    std::vector<Size> childSizes;
    double totalIntrinsicHeight = 0;
    LayoutConstraints childConstraints;
    childConstraints.availableWidth = contentRect.size.width;
    childConstraints.availableHeight = -1;

    for (size_t i = 0; i < m_flattened.size(); ++i) {
        auto result = m_flattened[i]->measure(ctx, childConstraints);
        childSizes.push_back(result.intrinsicSize);
        totalIntrinsicHeight += result.intrinsicSize.height;
        if (i + 1 < m_flattened.size())
            totalIntrinsicHeight += m_space;
    }

    double extraSpace = contentRect.size.height - totalIntrinsicHeight;
    double offsetY = contentRect.pos.y;

    // Compute start offset and spacing based on main-align
    double spacing = m_space;
    if (m_mainAlign == "center") {
        offsetY += extraSpace / 2.0;
    } else if (m_mainAlign == "end") {
        offsetY += extraSpace;
    } else if (m_mainAlign == "space-between" && m_flattened.size() > 1) {
        spacing = m_space + extraSpace / (static_cast<double>(m_flattened.size()) - 1);
    } else if (m_mainAlign == "space-around" && m_flattened.size() > 0) {
        double perItem = extraSpace / static_cast<double>(m_flattened.size());
        offsetY += perItem / 2.0;
        spacing = m_space + perItem;
    } else if (m_mainAlign == "space-evenly" && m_flattened.size() > 0) {
        double perGap = extraSpace / static_cast<double>(m_flattened.size() + 1);
        offsetY += perGap;
        spacing = m_space + perGap;
    }

    for (size_t i = 0; i < m_flattened.size(); ++i) {
        double childHeight = childSizes[i].height;
        double childWidth = childSizes[i].width;

        // Cross-axis alignment determines width
        if (m_crossAlign == "stretch") {
            childWidth = contentRect.size.width;
        }

        double childX = contentRect.pos.x;
        if (m_crossAlign == "center") {
            childX = contentRect.pos.x + (contentRect.size.width - childWidth) / 2.0;
        } else if (m_crossAlign == "end") {
            childX = contentRect.pos.x + contentRect.size.width - childWidth;
        }

        Rect childRect;
        childRect.pos.x = childX;
        childRect.pos.y = offsetY;
        childRect.size.width = childWidth;
        childRect.size.height = childHeight;

        m_flattened[i]->layout(ctx, childRect);
        offsetY += childHeight + spacing;
    }
}

void ColumnLayout::render(QPainter* painter, const LayoutContext& ctx) const
{
    ContainerElement::render(painter, ctx);
    for (const auto& child : m_flattened) {
        child->render(painter, ctx);
    }
}

bool ColumnLayout::bindsProperty(const QString& name) const
{
    if (ContainerElement::bindsProperty(name))
        return true;
    for (const auto& child : m_children) {
        if (child && child->bindsProperty(name))
            return true;
    }
    return false;
}

} // namespace BroadItem
