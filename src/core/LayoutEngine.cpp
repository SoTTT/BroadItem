#include <broaditem/core/LayoutEngine.h>
#include <broaditem/element/Element.h>
#include <QPainter>

namespace BroadItem {

/// @brief 物化模板元素树为实例节点树。
/// @param root 模板树根元素。
/// @param ctx 布局上下文，提供属性绑定。
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
///
/// 经 node.element 派发（而非由调用方另传模板根）：根模板为控制元素时，
/// 实例根节点由被展开的子模板产生，只有 node.element 保证指向可渲染元素。
///
/// @param ctx 布局上下文，提供属性绑定。
/// @param constraints 可用宽高约束。
/// @param node 根实例节点。
/// @return 元素树的固有尺寸；node.element 为空时返回 (0,0)。
QSizeF LayoutEngine::measure(const LayoutContext& ctx, const LayoutConstraints& constraints, Node& node)
{
    if (!node.element)
        return QSizeF(0, 0);
    return node.element->measure(ctx, constraints, node).intrinsicSize;
}

/// @brief 在给定矩形内为树中所有节点分配位置（派发语义同 measure）。
/// @param ctx 布局上下文，提供属性绑定。
/// @param rect 布局目标矩形。
/// @param node 根实例节点。
void LayoutEngine::layout(const LayoutContext& ctx, const QRectF& rect, Node& node)
{
    if (node.element)
        node.element->layout(ctx, rect, node);
}

/// @brief 将实例节点树渲染到 painter 上（派发语义同 measure）。
/// @param painter 目标 QPainter。
/// @param ctx 布局上下文，提供属性绑定。
/// @param node 根实例节点。
void LayoutEngine::render(QPainter* painter, const LayoutContext& ctx, const Node& node)
{
    if (node.element)
        node.element->render(painter, ctx, node);
}

} // namespace BroadItem
