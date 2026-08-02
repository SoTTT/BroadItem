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

/// @brief 协调实例节点树的物化 + 三阶段流水线（测量、布局、渲染）。
///
/// 物化以模板树根为输入；测量/布局/渲染只作用于实例节点树，
/// 经 Node::element 派发到产生该节点的模板元素（与 Frame 管线语义一致：
/// 根模板为控制元素时，实例根节点的 element 是被展开的子模板而非根模板自身）。
class LayoutEngine {
public:
    /// @brief 物化模板树：返回实例节点树根。
    /// 根为控制元素时取展开结果的第一个节点；无法物化时返回 nullptr。
    static std::unique_ptr<Node> materialize(const ElementPtr& root, const LayoutContext& ctx);
    /// @brief 在根节点上运行测量阶段，计算固有尺寸。
    static QSizeF measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node);
    /// @brief 运行布局阶段，在给定的矩形内分配位置和尺寸。
    static void layout(const LayoutContext& ctx, const QRectF& rect, Node& node);
    /// @brief 运行渲染阶段，将实例节点树绘制到给定的 painter 上。
    static void render(QPainter* painter, const LayoutContext& ctx, const Node& node);
};

} // namespace BroadItem
