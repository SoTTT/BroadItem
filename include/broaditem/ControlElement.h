#pragma once

#include "Element.h"

namespace BroadItem {

// ControlElement: elements that do NOT participate in rendering directly.
// They expand into cloned instances at runtime via expand().
// Per design doc, control elements do NOT have decorators.
class ControlElement : public Element {
public:
    virtual ~ControlElement() = default;

    // Default no-op implementations for control elements
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override {
        Q_UNUSED(ctx)
        Q_UNUSED(constraints)
        return MeasureResult{Size{0, 0}};
    }
    void layout(const LayoutContext& ctx, const Rect& rect) override {
        Q_UNUSED(ctx)
        Q_UNUSED(rect)
    }
    void render(QPainter* painter, const LayoutContext& ctx) const override {
        Q_UNUSED(painter)
        Q_UNUSED(ctx)
    }
};

} // namespace BroadItem
