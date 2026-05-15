#pragma once

#include "../ControlElement.h"

namespace BroadItem {

// IfHasElement is a control element that conditionally renders its child.
// It does NOT participate in layout directly; instead, the parent container
// calls expand() to get either {cloned_child} or {}.
class IfHasElement : public ControlElement {
public:
    void parse(const QDomElement& xml) override;
    bool bindsProperty(const QString& name) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    void setBindProperty(const QString& bind);
    void setNot(bool notValue);
    void setChild(ElementPtr child);

    ElementPtr clone() const override;

    // Expand this if-has element into either { cloned_child } or {}.
    std::vector<ElementPtr> expand(const LayoutContext& ctx) const;

    // Override to delegate to child (when used as root element)
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const Rect& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;

private:
    QString m_propertyName;
    bool m_not = false;
    ElementPtr m_child;

    bool shouldShow(const LayoutContext& ctx) const;
};

} // namespace BroadItem
