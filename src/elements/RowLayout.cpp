#include "broaditem/elements/RowLayout.h"
#include "broaditem/elements/ForElement.h"
#include "broaditem/elements/IfHasElement.h"
#include "broaditem/SizedElement.h"
#include <algorithm>
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

const QSet<QString>& RowLayout::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{"main-align", "cross-align", "space"} + boxModelAttributeNames();
    return attrs;
}

void RowLayout::parse(const QDomElement& xml)
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

void RowLayout::addChild(ElementPtr child)
{
    m_children.push_back(std::move(child));
}

ElementPtr RowLayout::clone() const
{
    auto copy = std::make_shared<RowLayout>();
    copy->m_margin = m_margin;
    copy->m_border = m_border;
    copy->m_background = m_background;
    copy->m_padding = m_padding;
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

void RowLayout::interpolateValues(const QStringList& values)
{
    for (const auto& child : m_children) {
        if (child)
            child->interpolateValues(values);
    }
}

std::vector<ElementPtr> RowLayout::flattenChildren(const LayoutContext& ctx) const
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

MeasureResult RowLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    double totalWidth = 0;
    double maxHeight = 0;

    LayoutConstraints childConstraints = constraints;
    double decoW = boxModelWidth();
    double decoH = boxModelHeight();

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    m_flattened = flattenChildren(ctx);
    for (size_t i = 0; i < m_flattened.size(); ++i) {
        auto result = m_flattened[i]->measure(ctx, childConstraints);
        totalWidth += result.intrinsicSize.width();
        maxHeight = std::max(maxHeight, result.intrinsicSize.height());
        if (i + 1 < m_flattened.size())
            totalWidth += m_space;
    }

    QSizeF sz(totalWidth + decoW, maxHeight + decoH);
    return MeasureResult{sz};
}

void RowLayout::layout(const LayoutContext& ctx, const QRectF& rect)
{
    ContainerElement::layout(ctx, rect);

    QRectF cr = contentRect(rect);

    m_flattened = flattenChildren(ctx);
    layoutChildren(ctx, cr);
}

void RowLayout::layoutChildren(const LayoutContext& ctx, const QRectF& contentRect)
{
    if (m_flattened.empty())
        return;

    std::vector<QSizeF> childSizes;
    double totalIntrinsicWidth = 0;
    LayoutConstraints childConstraints;
    childConstraints.availableWidth = -1;
    childConstraints.availableHeight = contentRect.height();

    for (size_t i = 0; i < m_flattened.size(); ++i) {
        auto result = m_flattened[i]->measure(ctx, childConstraints);
        childSizes.push_back(result.intrinsicSize);
        totalIntrinsicWidth += result.intrinsicSize.width();
        if (i + 1 < m_flattened.size())
            totalIntrinsicWidth += m_space;
    }

    double extraSpace = contentRect.width() - totalIntrinsicWidth;
    double offsetX = contentRect.x();

    double spacing = m_space;
    if (m_mainAlign == "center") {
        offsetX += extraSpace / 2.0;
    } else if (m_mainAlign == "end") {
        offsetX += extraSpace;
    } else if (m_mainAlign == "space-between" && m_flattened.size() > 1) {
        spacing = m_space + extraSpace / (static_cast<double>(m_flattened.size()) - 1);
    } else if (m_mainAlign == "space-around" && !m_flattened.empty()) {
        double perItem = extraSpace / static_cast<double>(m_flattened.size());
        offsetX += perItem / 2.0;
        spacing = m_space + perItem;
    } else if (m_mainAlign == "space-evenly" && !m_flattened.empty()) {
        double perGap = extraSpace / static_cast<double>(m_flattened.size() + 1);
        offsetX += perGap;
        spacing = m_space + perGap;
    }

    for (size_t i = 0; i < m_flattened.size(); ++i) {
        double childWidth = childSizes[i].width();
        double childHeight = childSizes[i].height();

        if (m_crossAlign == "stretch") {
            auto* sized = dynamic_cast<SizedElement*>(m_flattened[i].get());
            if (!sized || !sized->hasHeight())
                childHeight = contentRect.height();
        }

        double childY = contentRect.y();
        if (m_crossAlign == "center") {
            childY = contentRect.y() + (contentRect.height() - childHeight) / 2.0;
        } else if (m_crossAlign == "end") {
            childY = contentRect.y() + contentRect.height() - childHeight;
        }

        QRectF childRect(offsetX, childY, childWidth, childHeight);
        m_flattened[i]->layout(ctx, childRect);
        offsetX += childWidth + spacing;
    }
}

void RowLayout::render(QPainter* painter, const LayoutContext& ctx) const
{
    ContainerElement::render(painter, ctx);
    for (const auto& child : m_flattened) {
        child->render(painter, ctx);
    }
}

bool RowLayout::bindsProperty(const QString& name) const
{
    if (ContainerElement::bindsProperty(name))
        return true;
    return std::any_of(m_children.begin(), m_children.end(),
        [&](const auto& child) { return child && child->bindsProperty(name); });
}

} // namespace BroadItem
