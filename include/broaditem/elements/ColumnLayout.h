#pragma once

#include "../ContainerElement.h"

namespace BroadItem {

/// @brief 垂直列布局，从上到下堆叠子元素。
class ColumnLayout : public ContainerElement {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    /// @brief 向此列添加子元素。
    void addChild(ElementPtr child);
    /// @brief 返回子元素之间的间距。
    double space() const { return m_space; }
    /// @brief 返回缓存的展平子元素列表。
    const std::vector<ElementPtr>& flattenedChildren() const { return m_flattened; }

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    ElementPtr clone() const override;

private:
    std::vector<ElementPtr> m_children;  ///< Direct child elements.
    QString m_mainAlign = "start";       ///< Main-axis alignment ("start", "center", "end").
    QString m_crossAlign = "stretch";    ///< Cross-axis alignment ("start", "center", "end", "stretch").
    double m_space = 0;                  ///< Spacing between children in pixels.

    mutable std::vector<ElementPtr> m_flattened; ///< Cached flattened children, populated during measure/layout.

    /// @brief 在计算出的内容矩形内布局子元素。
    void layoutChildren(const LayoutContext& ctx, const QRectF& contentRect);

    /// @brief 将控制元素展平为展开后的子元素。
    std::vector<ElementPtr> flattenChildren(const LayoutContext& ctx) const;
};

} // namespace BroadItem
