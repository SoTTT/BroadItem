#pragma once

#include "../Element.h"

namespace BroadItem {

// IfHasElement is a control element that conditionally renders its child.
// It does NOT participate in layout directly; instead, the parent container
// calls expand() to get either {cloned_child} or {}.
class IfHasElement : public Element {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const Rect& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    void setBindProperty(const QString& bind);
    void setNot(bool notValue);
    void setChild(ElementPtr child);

    ElementPtr clone() const override;

    // Expand this if-has element into either { cloned_child } or {}.
    std::vector<ElementPtr> expand(const LayoutContext& ctx) const;

private:
    QString m_propertyName;
    bool m_not = false;
    ElementPtr m_child;

    bool shouldShow(const LayoutContext& ctx) const;
};

} // namespace BroadItem
