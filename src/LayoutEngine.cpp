#include "broaditem/LayoutEngine.h"
#include "broaditem/Element.h"
#include <QPainter>

namespace BroadItem {

Size LayoutEngine::measure(const ElementPtr& root, const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    if (!root)
        return Size{0, 0};
    auto result = root->measure(ctx, constraints);
    return result.intrinsicSize;
}

void LayoutEngine::layout(const ElementPtr& root, const LayoutContext& ctx, const Rect& rect)
{
    if (root)
        root->layout(ctx, rect);
}

void LayoutEngine::render(const ElementPtr& root, QPainter* painter, const LayoutContext& ctx)
{
    if (root)
        root->render(painter, ctx);
}

} // namespace BroadItem
