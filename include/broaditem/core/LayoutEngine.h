#pragma once

#include <broaditem/context/LayoutContext.h>
#include <broaditem/core/Node.h>
#include <QRectF>
#include <QSizeF>
#include <QPainter>
#include <memory>

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

/// @brief 协调实例节点树的三阶段流水线（物化、测量、布局、渲染）。
class LayoutEngine {
public:
    /// @brief 物化模板树：返回实例节点树根。
    /// 根为控制元素时取展开结果的第一个节点；无法物化时返回 nullptr。
    static std::unique_ptr<Node> materialize(const ElementPtr& root, const LayoutContext& ctx);
    /// @brief 在根节点上运行测量阶段，计算固有尺寸。
    static QSizeF measure(const ElementPtr& root, const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node);
    /// @brief 运行布局阶段，在给定的矩形内分配位置和尺寸。
    static void layout(const ElementPtr& root, const LayoutContext& ctx, const QRectF& rect, Node& node);
    /// @brief 运行渲染阶段，将实例节点树绘制到给定的 painter 上。
    static void render(const ElementPtr& root, QPainter* painter, const LayoutContext& ctx, const Node& node);
};

} // namespace BroadItem
