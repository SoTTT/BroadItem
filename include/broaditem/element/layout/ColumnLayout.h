#pragma once

#include <broaditem/element/layout/MultiChildContainer.h>

namespace BroadItem {

/// @brief 垂直列布局，从上到下堆叠子元素。
class ColumnLayout : public MultiChildContainer {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node) const override;
    void layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const override;

    /// @brief 返回子元素之间的间距。
    double space() const { return m_space; }

    const QSet<QString>& supportedAttributes() const override;

private:
    QString m_mainAlign = "start";       ///< 主轴对齐（"start"、"center"、"end"）。
    QString m_crossAlign = "stretch";    ///< 交叉轴对齐（"start"、"center"、"end"、"stretch"）。
    double m_space = 0;                  ///< 子元素间距（px）。
    bool m_mainStretch = false;          ///< 是否将所有子元素拉伸为相同的主轴尺寸。

    /// @brief 在计算出的内容矩形内布局子节点。
    void layoutChildren(const LayoutContext& ctx, const QRectF& contentRect, Node& node) const;
};

} // namespace BroadItem
