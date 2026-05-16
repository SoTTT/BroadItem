#pragma once

#include "LayoutContext.h"
#include <QRectF>
#include <QSizeF>
#include <QPainter>

namespace BroadItem {

class Element;
using ElementPtr = std::shared_ptr<Element>;

class LayoutEngine {
public:
    static QSizeF measure(const ElementPtr& root, const LayoutContext& ctx, const LayoutConstraints& constraints);
    static void layout(const ElementPtr& root, const LayoutContext& ctx, const QRectF& rect);
    static void render(const ElementPtr& root, QPainter* painter, const LayoutContext& ctx);
};

} // namespace BroadItem
