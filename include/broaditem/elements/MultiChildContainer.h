#pragma once

#include "../ContainerElement.h"

namespace BroadItem {

class MultiChildContainer : public ContainerElement {
public:
    // Override from ContainerElement
    void resolveBindings(const LayoutContext& ctx) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    void addChild(ElementPtr child);
    const std::vector<ElementPtr>& flattenedChildren() const { return m_flattened; }
    bool canHaveChildren() const override { return true; }

    /// @brief Virtual hook for subclasses to validate children before adding.
    /// GridLayout overrides this to validate cell count.
    virtual bool validateChild(const ElementPtr& child) const { Q_UNUSED(child); return true; }

protected:
    std::vector<ElementPtr> m_children;
    mutable std::vector<ElementPtr> m_flattened;

    /// @brief Flatten control elements (ForElement, IfHasElement) into concrete children.
    std::vector<ElementPtr> flattenChildren(const LayoutContext& ctx) const;
};

} // namespace BroadItem
