#include <broaditem/element/layout/GridLayout.h>
#include <broaditem/element/layout/CellElement.h>
#include <broaditem/diagnostics/Diagnostics.h>
#include <broaditem/compat/QtCompat.h>
#include <algorithm>
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 GridLayout 支持的 XML 属性集合。
/// @return 属性名的静态集合引用。
const QSet<QString>& GridLayout::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{"columns", "rows", "space", "space-row", "space-column"} + boxModelAttributeNames();
    return attrs;
}

/// @brief 解析网格尺寸（列、行）和间距属性。
/// @param xml 要解析的 DOM 元素。
void GridLayout::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    validateAttributes(xml);
    if (hasLiteralAttribute(xml, "columns")) {
        if (validateInt(literalAttribute(xml, "columns"), "columns", m_columns)) {
            if (m_columns <= 0) {
                Diagnostics::reportParse(ErrorCode::LiteralOutOfRange,
                                         QStringLiteral("GridLayout: columns must be positive, got %1")
                                             .arg(m_columns));
                m_columns = 1;
            }
        }
    }
    if (hasLiteralAttribute(xml, "rows")) {
        if (validateInt(literalAttribute(xml, "rows"), "rows", m_rows)) {
            if (m_rows <= 0) {
                Diagnostics::reportParse(ErrorCode::LiteralOutOfRange,
                                         QStringLiteral("GridLayout: rows must be positive, got %1")
                                             .arg(m_rows));
                m_rows = 1;
            }
        }
    }
    if (hasLiteralAttribute(xml, "space"))
        validateDouble(literalAttribute(xml, "space"), "space", m_space);
    if (hasLiteralAttribute(xml, "space-row")) {
        double val = 0;
        if (validateDouble(literalAttribute(xml, "space-row"), "space-row", val))
            m_rowSpace = val;
    }
    if (hasLiteralAttribute(xml, "space-column")) {
        double val = 0;
        if (validateDouble(literalAttribute(xml, "space-column"), "space-column", val))
            m_columnSpace = val;
    }
}

/// @brief 解析期收尾校验：子元素必须全为 \<cell\> 且数量等于 columns×rows
///        （BI-P-008/009 由 parser 迁入；cell 判定的一处 dynamic_cast 局限在本类内部）。
/// @return 校验通过返回 true。
bool GridLayout::validateChildren() const
{
    int cellCount = 0;
    for (const auto& child : m_children) {
        if (std::dynamic_pointer_cast<CellElement>(child)) {
            cellCount++;
        } else {
            Diagnostics::reportParse(ErrorCode::GridNonCellChild,
                                     QStringLiteral("GridLayout: child must be <cell>"));
            return false;
        }
    }
    const int expected = m_columns * m_rows;
    if (cellCount != expected) {
        Diagnostics::reportParse(ErrorCode::GridCellCountMismatch,
                                 QStringLiteral("GridLayout: expected %1 cells (%2x%3), got %4")
                                     .arg(expected).arg(m_columns).arg(m_rows).arg(cellCount));
        return false;
    }
    return true;
}

/// @brief 物化：创建 GridNode 并拼接所有模板子元素的物化结果。
/// @param ctx 布局上下文。
/// @return 新创建的 GridNode 实例节点。
std::unique_ptr<Node> GridLayout::materialize(const LayoutContext& ctx) const
{
    auto node = makeUnique<GridNode>();
    node->element = this;
    resolveStyle(ctx, node->style);
    materializeChildrenInto(ctx, *node);
    return node;
}

/// @brief 测量网格：从子节点尺寸计算列宽和行高，缓存进 GridNode（并置位 measured 标记）。
/// @param ctx 布局上下文。
/// @param constraints 可用宽高约束。
/// @param node 实例节点（GridNode）。
/// @return 含盒模型装饰的网格测量尺寸。
MeasureResult GridLayout::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const
{
    auto& gridNode = static_cast<GridNode&>(node);

    double decoW = boxModelWidth(node.style);
    double decoH = boxModelHeight(node.style);

    LayoutConstraints childConstraints = constraints;
    if (constraints.availableWidth)
        childConstraints.availableWidth = std::max(0.0, *constraints.availableWidth - decoW);
    if (constraints.availableHeight)
        childConstraints.availableHeight = std::max(0.0, *constraints.availableHeight - decoH);

    gridNode.colWidths.assign(m_columns, 0.0);
    gridNode.rowHeights.assign(m_rows, 0.0);

    const auto& children = node.children;
    for (size_t i = 0; i < children.size(); ++i) {
        auto result = children[i]->element->measure(ctx, childConstraints, *children[i]);
        int col = static_cast<int>(i) % m_columns;
        int row = static_cast<int>(i) / m_columns;
        // col 恒小于 m_columns（取模结果）；超出网格容量（row >= m_rows）的子节点只测不排。
        gridNode.colWidths[col] = std::max(gridNode.colWidths[col], result.intrinsicSize.width());
        if (row < m_rows)
            gridNode.rowHeights[row] = std::max(gridNode.rowHeights[row], result.intrinsicSize.height());
    }

    double totalWidth = 0;
    for (double w : gridNode.colWidths)
        totalWidth += w;
    totalWidth += (m_columns - 1) * columnSpace();

    double totalHeight = 0;
    for (double h : gridNode.rowHeights)
        totalHeight += h;
    totalHeight += (m_rows - 1) * rowSpace();

    QSizeF sz(totalWidth + decoW, totalHeight + decoH);
    gridNode.measured = true;
    return MeasureResult{sz};
}

/// @brief 以网格模式定位子节点，均匀分配额外空间。
/// @param ctx 布局上下文。
/// @param rect 分配给此网格的矩形。
/// @param node 实例节点（GridNode，列宽/行高已在测量阶段缓存）。
void GridLayout::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    auto& gridNode = static_cast<GridNode&>(node);

    // 时序守卫：缓存须由 measure 先行填充（管线正常路径恒成立，
    // 乱序直调 LayoutEngine 时在此显式失败，而非空 vector 越界 UB）
    Q_ASSERT(gridNode.measured);

    node.rect = rect;

    QRectF cr = contentRect(rect, node.style);

    // 拷出局部副本分配剩余空间：缓存保持 measure 的纯输出不被改写，
    // 使同一 measure 之后 layout 可重入（重复 layout 不会重复膨胀列宽）。
    std::vector<double> colWidths = gridNode.colWidths;
    std::vector<double> rowHeights = gridNode.rowHeights;

    double measuredWidth = 0;
    for (double w : colWidths)
        measuredWidth += w;
    measuredWidth += (m_columns - 1) * columnSpace();

    double measuredHeight = 0;
    for (double h : rowHeights)
        measuredHeight += h;
    measuredHeight += (m_rows - 1) * rowSpace();

    double extraW = cr.width() - measuredWidth;
    double extraH = cr.height() - measuredHeight;

    if (extraW > 0 && m_columns > 0) {
        double add = extraW / m_columns;
        for (double& w : colWidths)
            w += add;
    }
    if (extraH > 0 && m_rows > 0) {
        double add = extraH / m_rows;
        for (double& h : rowHeights)
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

            QRectF cellRect(x, y, colWidths[col], rowHeights[row]);
            children[idx]->element->layout(ctx, cellRect, *children[idx]);
            x += colWidths[col] + columnSpace();
        }
        y += rowHeights[row] + rowSpace();
    }
}

} // namespace BroadItem
