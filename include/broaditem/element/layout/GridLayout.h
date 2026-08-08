#pragma once

#include <broaditem/element/layout/MultiChildContainer.h>
#include <broaditem/compat/Optional.h>

namespace BroadItem {

/// @brief GridLayout 的实例节点，缓存测量阶段的列宽/行高供布局阶段使用。
struct GridNode : Node {
    std::vector<double> colWidths;          ///< 计算得出的各列宽度（measure 的纯输出，layout 不改写）。
    std::vector<double> rowHeights;         ///< 计算得出的各行高度（同上）。
    bool measured = false;                  ///< measure 已填充缓存的标记（layout 入口断言用）。
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
    /// @brief 解析期收尾校验：子元素必须全为 \<cell\>（BI-P-008）且数量等于 columns×rows（BI-P-009）。
    bool validateChildren() const override;

private:
    int m_columns = 1;                    ///< 网格列数。
    int m_rows = 1;                       ///< 网格行数。
    double m_space = 0;                   ///< 所有单元格之间的默认间距。
    Optional<double> m_rowSpace;          ///< 行间距（设置后覆盖 m_space）。
    Optional<double> m_columnSpace;       ///< 列间距（设置后覆盖 m_space）。
};

} // namespace BroadItem
