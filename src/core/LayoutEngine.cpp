#include <broaditem/core/LayoutEngine.h>
#include <broaditem/element/Element.h>
#include <QPainter>

namespace BroadItem {

/// @brief 测量元素树的固有尺寸。
/// @param root The root element to measure.
/// @param ctx The layout context providing property bindings.
/// @param constraints Available width/height constraints.
/// @return The intrinsic size of the element tree.
QSizeF LayoutEngine::measure(const ElementPtr& root, const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    if (!root)
        return QSizeF(0, 0);
    auto result = root->measure(ctx, constraints);
    return result.intrinsicSize;
}

/// @brief 在给定矩形内为树中所有元素分配位置。
/// @param root The root element to lay out.
/// @param ctx The layout context providing property bindings.
/// @param rect The bounding rectangle to lay out within.
void LayoutEngine::layout(const ElementPtr& root, const LayoutContext& ctx, const QRectF& rect)
{
    if (root)
        root->layout(ctx, rect);
}

/// @brief 将元素树渲染到 painter 上。
/// @param root The root element to render.
/// @param painter The QPainter to render onto.
/// @param ctx The layout context providing property bindings.
void LayoutEngine::render(const ElementPtr& root, QPainter* painter, const LayoutContext& ctx)
{
    if (root)
        root->render(painter, ctx);
}

} // namespace BroadItem
