#include "broaditem/elements/GridLayout.h"
#include "broaditem/elements/CellElement.h"
#include "broaditem/elements/ForElement.h"
#include "broaditem/elements/IfHasElement.h"
#include <QPainter>
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

void GridLayout::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    if (xml.hasAttribute("columns")) {
        m_columns = xml.attribute("columns").toInt();
        if (m_columns <= 0) {
            qWarning() << "GridLayout: columns must be positive, got" << m_columns;
            m_columns = 1;
        }
    }
    if (xml.hasAttribute("rows")) {
        m_rows = xml.attribute("rows").toInt();
        if (m_rows <= 0) {
            qWarning() << "GridLayout: rows must be positive, got" << m_rows;
            m_rows = 1;
        }
    }
    if (xml.hasAttribute("space"))
        m_space = parseDouble(xml.attribute("space"));
    if (xml.hasAttribute("space-row"))
        m_rowSpace = parseDouble(xml.attribute("space-row"));
    if (xml.hasAttribute("space-column"))
        m_columnSpace = parseDouble(xml.attribute("space-column"));

    if (m_rowSpace == 0) m_rowSpace = m_space;
    if (m_columnSpace == 0) m_columnSpace = m_space;
}

void GridLayout::addChild(ElementPtr child)
{
    m_children.push_back(child);
}

ElementPtr GridLayout::clone() const
{
    auto copy = std::make_shared<GridLayout>();
    copy->decorators = decorators;
    copy->m_rect = m_rect;
    copy->m_columns = m_columns;
    copy->m_rows = m_rows;
    copy->m_space = m_space;
    copy->m_rowSpace = m_rowSpace;
    copy->m_columnSpace = m_columnSpace;
    for (const auto& child : m_children) {
        if (child)
            copy->m_children.push_back(child->clone());
    }
    return copy;
}

void GridLayout::interpolateValues(const QStringList& values)
{
    for (const auto& child : m_children) {
        if (child)
            child->interpolateValues(values);
    }
}

std::vector<ElementPtr> GridLayout::flattenChildren(const LayoutContext& ctx) const
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

MeasureResult GridLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    double decoW = decorators.totalWidth();
    double decoH = decorators.totalHeight();

    LayoutConstraints childConstraints = constraints;
    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    m_colWidths.assign(m_columns, 0.0);
    m_rowHeights.assign(m_rows, 0.0);
    m_cellMeasures.clear();

    m_flattened = flattenChildren(ctx);
    for (size_t i = 0; i < m_flattened.size(); ++i) {
        auto result = m_flattened[i]->measure(ctx, childConstraints);
        m_cellMeasures.push_back({result.intrinsicSize.width, result.intrinsicSize.height});
        int col = static_cast<int>(i) % m_columns;
        int row = static_cast<int>(i) / m_columns;
        if (col < m_columns)
            m_colWidths[col] = std::max(m_colWidths[col], result.intrinsicSize.width);
        if (row < m_rows)
            m_rowHeights[row] = std::max(m_rowHeights[row], result.intrinsicSize.height);
    }

    double totalWidth = 0;
    for (double w : m_colWidths)
        totalWidth += w;
    totalWidth += (m_columns - 1) * m_columnSpace;

    double totalHeight = 0;
    for (double h : m_rowHeights)
        totalHeight += h;
    totalHeight += (m_rows - 1) * m_rowSpace;

    Size sz;
    sz.width = totalWidth + decoW;
    sz.height = totalHeight + decoH;
    return MeasureResult{sz};
}

void GridLayout::layout(const LayoutContext& ctx, const Rect& rect)
{
    ContainerElement::layout(ctx, rect);

    Rect contentRect;
    contentRect.pos.x = rect.pos.x + decorators.margin.left + decorators.border.width + decorators.padding.left;
    contentRect.pos.y = rect.pos.y + decorators.margin.top + decorators.border.width + decorators.padding.top;
    contentRect.size.width = std::max(0.0, rect.size.width - decorators.totalWidth());
    contentRect.size.height = std::max(0.0, rect.size.height - decorators.totalHeight());

    // Distribute extra space proportionally
    double measuredWidth = 0;
    for (double w : m_colWidths)
        measuredWidth += w;
    measuredWidth += (m_columns - 1) * m_columnSpace;

    double measuredHeight = 0;
    for (double h : m_rowHeights)
        measuredHeight += h;
    measuredHeight += (m_rows - 1) * m_rowSpace;

    double extraW = contentRect.size.width - measuredWidth;
    double extraH = contentRect.size.height - measuredHeight;

    if (extraW > 0 && m_columns > 0) {
        double add = extraW / m_columns;
        for (double& w : m_colWidths)
            w += add;
    }
    if (extraH > 0 && m_rows > 0) {
        double add = extraH / m_rows;
        for (double& h : m_rowHeights)
            h += add;
    }

    double y = contentRect.pos.y;
    for (int row = 0; row < m_rows; ++row) {
        double x = contentRect.pos.x;
        for (int col = 0; col < m_columns; ++col) {
            int idx = row * m_columns + col;
            if (idx >= static_cast<int>(m_flattened.size()))
                break;

            Rect cellRect;
            cellRect.pos.x = x;
            cellRect.pos.y = y;
            cellRect.size.width = m_colWidths[col];
            cellRect.size.height = m_rowHeights[row];

            m_flattened[idx]->layout(ctx, cellRect);
            x += m_colWidths[col] + m_columnSpace;
        }
        y += m_rowHeights[row] + m_rowSpace;
    }
}

void GridLayout::render(QPainter* painter, const LayoutContext& ctx) const
{
    ContainerElement::render(painter, ctx);
    for (const auto& child : m_flattened) {
        child->render(painter, ctx);
    }
}

bool GridLayout::bindsProperty(const QString& name) const
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
