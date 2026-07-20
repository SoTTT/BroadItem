#include <broaditem/element/layout/GridLayout.h>
#include <algorithm>
#include <QPainter>
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

/// @brief 返回 GridLayout 支持的 XML 属性集合。
/// @return Reference to a static set of attribute names.
const QSet<QString>& GridLayout::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{"columns", "rows", "space", "space-row", "space-column"} + boxModelAttributeNames();
    return attrs;
}

/// @brief 解析网格尺寸（列、行）和间距属性。
/// @param xml The DOM element to parse.
void GridLayout::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttribute("columns")) {
        if (validateInt(xml.attribute("columns"), "columns", m_columns)) {
            if (m_columns <= 0) {
                qWarning() << "GridLayout: columns must be positive, got" << m_columns;
                m_columns = 1;
            }
        }
    }
    if (xml.hasAttribute("rows")) {
        if (validateInt(xml.attribute("rows"), "rows", m_rows)) {
            if (m_rows <= 0) {
                qWarning() << "GridLayout: rows must be positive, got" << m_rows;
                m_rows = 1;
            }
        }
    }
    if (xml.hasAttribute("space"))
        validateDouble(xml.attribute("space"), "space", m_space);
    if (xml.hasAttribute("space-row")) {
        double val = 0;
        if (validateDouble(xml.attribute("space-row"), "space-row", val))
            m_rowSpace = val;
    }
    if (xml.hasAttribute("space-column")) {
        double val = 0;
        if (validateDouble(xml.attribute("space-column"), "space-column", val))
            m_columnSpace = val;
    }
}

/// @brief 向此网格添加子元素（通常是 CellElement），并验证单元格数量。
/// @param child The element to add.
void GridLayout::addChild(ElementPtr child)
{
    MultiChildContainer::addChild(std::move(child));
    if (static_cast<int>(m_children.size()) > m_columns * m_rows) {
        qWarning() << "GridLayout: too many children — maximum is" << (m_columns * m_rows);
    }
}

/// @brief 物化：创建 GridNode 并拼接所有模板子元素的物化结果。
/// @param ctx 布局上下文。
/// @return 新创建的 GridNode 实例节点。
std::unique_ptr<Node> GridLayout::materialize(const LayoutContext& ctx) const
{
    auto node = std::make_unique<GridNode>();
    node->element = this;
    resolveStyle(ctx, node->style);
    materializeChildrenInto(ctx, *node);
    return node;
}

/// @brief 测量网格：从子节点尺寸计算列宽和行高，缓存进 GridNode。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @param node 实例节点（GridNode）。
/// @return The measured grid size including box model decoration.
MeasureResult GridLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    auto& gridNode = static_cast<GridNode&>(node);

    double decoW = boxModelWidth(node.style);
    double decoH = boxModelHeight(node.style);

    LayoutConstraints childConstraints = constraints;
    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    gridNode.colWidths.assign(m_columns, 0.0);
    gridNode.rowHeights.assign(m_rows, 0.0);
    gridNode.cellMeasures.clear();

    const auto& children = node.children;
    for (size_t i = 0; i < children.size(); ++i) {
        auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
        gridNode.cellMeasures.push_back({result.intrinsicSize.width(), result.intrinsicSize.height()});
        int col = static_cast<int>(i) % m_columns;
        int row = static_cast<int>(i) / m_columns;
        if (col < m_columns)
            gridNode.colWidths[col] = std::max(gridNode.colWidths[col], result.intrinsicSize.width());
        if (row < m_rows)
            gridNode.rowHeights[row] = std::max(gridNode.rowHeights[row], result.intrinsicSize.height());
    }

    double totalWidth = 0;
    for (double w : gridNode.colWidths)
        totalWidth += w;
    totalWidth += (m_columns - 1) * m_columnSpace.value_or(m_space);

    double totalHeight = 0;
    for (double h : gridNode.rowHeights)
        totalHeight += h;
    totalHeight += (m_rows - 1) * m_rowSpace.value_or(m_space);

    QSizeF sz(totalWidth + decoW, totalHeight + decoH);
    return MeasureResult{sz};
}

/// @brief 以网格模式定位子节点，均匀分配额外空间。
/// @param ctx The layout context.
/// @param rect The bounding rectangle assigned to this grid.
/// @param node 实例节点（GridNode，列宽/行高已在测量阶段缓存）。
void GridLayout::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    auto& gridNode = static_cast<GridNode&>(node);

    node.rect = rect;

    QRectF cr = contentRect(rect, node.style);

    double measuredWidth = 0;
    for (double w : gridNode.colWidths)
        measuredWidth += w;
    measuredWidth += (m_columns - 1) * m_columnSpace.value_or(m_space);

    double measuredHeight = 0;
    for (double h : gridNode.rowHeights)
        measuredHeight += h;
    measuredHeight += (m_rows - 1) * m_rowSpace.value_or(m_space);

    double extraW = cr.width() - measuredWidth;
    double extraH = cr.height() - measuredHeight;

    if (extraW > 0 && m_columns > 0) {
        double add = extraW / m_columns;
        for (double& w : gridNode.colWidths)
            w += add;
    }
    if (extraH > 0 && m_rows > 0) {
        double add = extraH / m_rows;
        for (double& h : gridNode.rowHeights)
            h += add;
    }

    const auto& children = node.children;
    double y = cr.y();
    for (int row = 0; row < m_rows; ++row) {
        double x = cr.x();
        for (int col = 0; col < m_columns; ++col) {
            int idx = row * m_columns + col;
            if (idx >= static_cast<int>(children.size()))
                break;

            QRectF cellRect(x, y, gridNode.colWidths[col], gridNode.rowHeights[row]);
            children[idx]->element->layout(ctx, cellRect, *children[idx]);
            x += gridNode.colWidths[col] + m_columnSpace.value_or(m_space);
        }
        y += gridNode.rowHeights[row] + m_rowSpace.value_or(m_space);
    }
}

} // namespace BroadItem
