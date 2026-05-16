#pragma once

#include "../ControlElement.h"

namespace BroadItem {

// ForElement is a control element that expands into multiple instances at runtime.
// It does NOT participate in measure/layout/render directly; instead, the parent
// container calls expand() to get a list of cloned elements that participate in
// the parent's unified layout.
class ForElement : public ControlElement {
public:
    void parse(const QDomElement& xml) override;
    bool bindsProperty(const QString& name) const override;

    void setBindProperty(const QString& bind);
    void setTemplate(ElementPtr templ);
    ElementPtr templateElement() const { return m_template; }

    ElementPtr clone() const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    // Expand this for-element into a list of cloned element instances,
    // one per bound value, with interpolation applied.
    std::vector<ElementPtr> expand(const LayoutContext& ctx) const;

    // Override to delegate to expanded child (when used as root element)
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;

private:
    QString m_ofProperty;
    int m_step = 1;
    ElementPtr m_template;
};

} // namespace BroadItem
