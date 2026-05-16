#pragma once

#include "Element.h"
#include "BoxModel.h"

namespace BroadItem {

// RenderableElement: elements that participate in rendering with decorators.
// All non-control elements inherit from this class.
class RenderableElement : public Element {
public:
    virtual ~RenderableElement() = default;

    // Decorators (implicitly available to all renderable elements per design doc)
    Decorators decorators;

    // Parse decorator pseudo-attributes (margin-*, padding-*, border-*, background-*)
    void parseDecorators(const QDomElement& xml);

    // Render decorators (background, border) into the given rect
    void renderDecorators(QPainter* painter, const QRectF& rect) const;

    // Return the content rect inside decorators (removes margin/border/padding)
    QRectF contentRect(const QRectF& outerRect) const;

    // Return all decorator pseudo-attribute names
    static const QSet<QString>& decoratorAttributeNames();
};

} // namespace BroadItem
