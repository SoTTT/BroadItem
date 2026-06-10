#pragma once

#include "../ContainerElement.h"
#include <optional>

namespace BroadItem {

/// @brief 网格布局，在固定的行列网格中排列子元素。
class GridLayout : public ContainerElement {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    /// @brief 向此网格添加子元素。
    void addChild(ElementPtr child);
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
    bool canHaveChildren() const override { return true; }

    ElementPtr clone() const override;

    void resolveBindings(const LayoutContext& ctx) override;

private:
    std::vector<ElementPtr> m_children;  ///< Direct child elements.
    int m_columns = 1;                    ///< Number of columns in the grid.
    int m_rows = 1;                       ///< Number of rows in the grid.
    double m_space = 0;                   ///< Default spacing between all cells.
    std::optional<double> m_rowSpace;     ///< Spacing between rows (overrides m_space if set).
    std::optional<double> m_columnSpace;  ///< Spacing between columns (overrides m_space if set).

    struct CellMeasure {
        double width = 0;
        double height = 0;
    };
    std::vector<CellMeasure> m_cellMeasures;  ///< Per-cell measure results.
    std::vector<double> m_colWidths;           ///< Computed column widths.
    std::vector<double> m_rowHeights;          ///< Computed row heights.

    mutable std::vector<ElementPtr> m_flattened; ///< Cached flattened children, populated during measure/layout.

    /// @brief 将控制元素展平为展开后的子元素。
    std::vector<ElementPtr> flattenChildren(const LayoutContext& ctx) const;
};

} // namespace BroadItem
