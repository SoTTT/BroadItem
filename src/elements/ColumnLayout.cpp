#include "broaditem/elements/ColumnLayout.h"
#include "broaditem/elements/ForElement.h"
#include "broaditem/elements/IfHasElement.h"
#include "broaditem/SizedElement.h"
#include <QPainter>
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

const QSet<QString>& ColumnLayout::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{"main-align", "cross-align", "space"} + decoratorAttributeNames();
    return attrs;
}

void ColumnLayout::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttribute("main-align"))
        m_mainAlign = xml.attribute("main-align");
    if (xml.hasAttribute("cross-align"))
        m_crossAlign = xml.attribute("cross-align");
    if (xml.hasAttribute("space"))
        validateDouble(xml.attribute("space"), "space", m_space);
}

void ColumnLayout::addChild(ElementPtr child)
{
    m_children.push_back(std::move(child));
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
        totalHeight += result.intrinsicSize.height();
        maxWidth = std::max(maxWidth, result.intrinsicSize.width());
        if (i + 1 < m_flattened.size())
            totalHeight += m_space;
    }

    QSizeF sz(maxWidth + decoW, totalHeight + decoH);
    return MeasureResult{sz};
}

void ColumnLayout::layout(const LayoutContext& ctx, const QRectF& rect)
{
    ContainerElement::layout(ctx, rect);

    // Content rect after decorators
    QRectF contentRect(rect.x() + decorators.margin.left + decorators.border.width + decorators.padding.left,
                       rect.y() + decorators.margin.top + decorators.border.width + decorators.padding.top,
                       std::max(0.0, rect.width() - decorators.totalWidth()),
                       std::max(0.0, rect.height() - decorators.totalHeight()));

    m_flattened = flattenChildren(ctx);
    layoutChildren(ctx, contentRect);
}

void ColumnLayout::layoutChildren(const LayoutContext& ctx, const QRectF& contentRect)
{
    if (m_flattened.empty())
        return;

    // Measure all children
    std::vector<QSizeF> childSizes;
    double totalIntrinsicHeight = 0;
    LayoutConstraints childConstraints;
    childConstraints.availableWidth = contentRect.width();
    childConstraints.availableHeight = -1;

    for (size_t i = 0; i < m_flattened.size(); ++i) {
        auto result = m_flattened[i]->measure(ctx, childConstraints);
        childSizes.push_back(result.intrinsicSize);
        totalIntrinsicHeight += result.intrinsicSize.height();
        if (i + 1 < m_flattened.size())
            totalIntrinsicHeight += m_space;
    }

    double extraSpace = contentRect.height() - totalIntrinsicHeight;
    double offsetY = contentRect.y();

    // Compute start offset and spacing based on main-align
    double spacing = m_space;
    if (m_mainAlign == "center") {
        offsetY += extraSpace / 2.0;
    } else if (m_mainAlign == "end") {
        offsetY += extraSpace;
    } else if (m_mainAlign == "space-between" && m_flattened.size() > 1) {
        spacing = m_space + extraSpace / (static_cast<double>(m_flattened.size()) - 1);
    } else if (m_mainAlign == "space-around" && !m_flattened.empty()) {
        double perItem = extraSpace / static_cast<double>(m_flattened.size());
        offsetY += perItem / 2.0;
        spacing = m_space + perItem;
    } else if (m_mainAlign == "space-evenly" && !m_flattened.empty()) {
        double perGap = extraSpace / static_cast<double>(m_flattened.size() + 1);
        offsetY += perGap;
        spacing = m_space + perGap;
    }

    for (size_t i = 0; i < m_flattened.size(); ++i) {
        double childHeight = childSizes[i].height();
        double childWidth = childSizes[i].width();

        // Cross-axis alignment determines width
        if (m_crossAlign == "stretch") {
            auto* sized = dynamic_cast<SizedElement*>(m_flattened[i].get());
            if (!sized || !sized->hasWidth())
                childWidth = contentRect.width();
        }

        double childX = contentRect.x();
        if (m_crossAlign == "center") {
            childX = contentRect.x() + (contentRect.width() - childWidth) / 2.0;
        } else if (m_crossAlign == "end") {
            childX = contentRect.x() + contentRect.width() - childWidth;
        }

        QRectF childRect(childX, offsetY, childWidth, childHeight);
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
    return std::any_of(m_children.begin(), m_children.end(),
        [&](const auto& child) { return child && child->bindsProperty(name); });
}

} // namespace BroadItem
