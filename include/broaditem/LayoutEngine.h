#pragma once

#include "BoxModel.h"
#include "LayoutContext.h"

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

class LayoutEngine {
public:
    static Size measure(ElementPtr root, const LayoutContext& ctx, const LayoutConstraints& constraints);
    static void layout(ElementPtr root, const LayoutContext& ctx, const Rect& rect);
    static void render(ElementPtr root, QPainter* painter, const LayoutContext& ctx);
};

} // namespace BroadItem
