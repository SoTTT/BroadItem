#pragma once

#include "BoxModel.h"
#include "LayoutContext.h"

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

class LayoutEngine {
public:
    static Size measure(const ElementPtr& root, const LayoutContext& ctx, const LayoutConstraints& constraints);
    static void layout(const ElementPtr& root, const LayoutContext& ctx, const Rect& rect);
    static void render(const ElementPtr& root, QPainter* painter, const LayoutContext& ctx);
};

} // namespace BroadItem
