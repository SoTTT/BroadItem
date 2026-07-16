#include <broaditem/core/LayoutEngine.h>
#include <broaditem/element/Element.h>
#include <QPainter>

namespace BroadItem {

/// @brief 物化模板元素树为实例节点树。
/// @param root The root template element.
/// @param ctx The layout context providing property bindings.
/// @return 实例节点树根；root 为空或物化结果为空时返回 nullptr。
std::unique_ptr<Node> LayoutEngine::materialize(const ElementPtr& root, const LayoutContext& ctx)
{
    if (!root)
        return nullptr;
    auto nodes = root->materializeChildren(ctx);
    if (nodes.empty())
        return nullptr;
    return std::move(nodes[0]);
}

/// @brief 测量实例节点树的固有尺寸。
/// @param root The root template element.
/// @param ctx The layout context providing property bindings.
/// @param constraints Available width/height constraints.
/// @param node 根实例节点。
/// @return The intrinsic size of the element tree.
QSizeF LayoutEngine::measure(const ElementPtr& root, const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node)
{
    if (!root)
        return QSizeF(0, 0);
    auto result = root->measure(ctx, constraints, node);
    return result.intrinsicSize;
}

/// @brief 在给定矩形内为树中所有节点分配位置。
/// @param root The root template element.
/// @param ctx The layout context providing property bindings.
/// @param rect The bounding rectangle to lay out within.
/// @param node 根实例节点。
void LayoutEngine::layout(const ElementPtr& root, const LayoutContext& ctx, const QRectF& rect, Node& node)
{
    if (root)
        root->layout(ctx, rect, node);
}

/// @brief 将实例节点树渲染到 painter 上。
/// @param root The root template element.
/// @param painter The QPainter to render onto.
/// @param ctx The layout context providing property bindings.
/// @param node 根实例节点。
void LayoutEngine::render(const ElementPtr& root, QPainter* painter, const LayoutContext& ctx, const Node& node)
{
    if (root)
        root->render(painter, ctx, node);
}

} // namespace BroadItem
