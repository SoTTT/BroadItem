#pragma once

#include <broaditem/element/layout/MultiChildContainer.h>
#include <optional>

namespace BroadItem {

/// @brief GridLayout 的实例节点，缓存测量阶段的列宽/行高供布局阶段使用。
struct GridNode : Node {
    struct CellMeasure {
        double width = 0;
        double height = 0;
    };
    std::vector<CellMeasure> cellMeasures;  ///< Per-cell measure results.
    std::vector<double> colWidths;          ///< Computed column widths.
    std::vector<double> rowHeights;         ///< Computed row heights.
};

/// @brief 网格布局，在固定的行列网格中排列子元素。
class GridLayout : public MultiChildContainer {
public:
    void parse(const QDomElement& xml) override;
    std::unique_ptr<Node> materialize(const LayoutContext& ctx) const override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const override;
    void layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const override;

    /// @brief 返回直接子元素列表。
    const std::vector<ElementPtr>& children() const { return m_children; }

    /// @brief 返回列数。
    int columns() const { return m_columns; }
    /// @brief 返回行数。
    int rows() const { return m_rows; }
    /// @brief 返回列间距。
    double columnSpace() const { return m_columnSpace.value_or(m_space); }
    /// @brief 返回行间距。
    double rowSpace() const { return m_rowSpace.value_or(m_space); }

    const QSet<QString>& supportedAttributes() const override;

private:
    int m_columns = 1;                    ///< Number of columns in the grid.
    int m_rows = 1;                       ///< Number of rows in the grid.
    double m_space = 0;                   ///< Default spacing between all cells.
    std::optional<double> m_rowSpace;     ///< Spacing between rows (overrides m_space if set).
    std::optional<double> m_columnSpace;  ///< Spacing between columns (overrides m_space if set).
};

} // namespace BroadItem
