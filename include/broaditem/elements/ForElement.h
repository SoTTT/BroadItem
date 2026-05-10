#pragma once

#include "../Element.h"

namespace BroadItem {

// ForElement is a control element that expands into multiple instances at runtime.
// It does NOT participate in measure/layout/render directly; instead, the parent
// container calls expand() to get a list of cloned elements that participate in
// the parent's unified layout.
class ForElement : public Element {
public:
    void parse(const QDomElement& xml) override;
    bool bindsProperty(const QString& name) const override;

    // ForElement does not participate in layout directly.
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

    void setBindProperty(const QString& bind);
    void setTemplate(ElementPtr templ);
    ElementPtr templateElement() const { return m_template; }

    ElementPtr clone() const override;

    // Expand this for-element into a list of cloned element instances,
    // one per bound value, with interpolation applied.
    std::vector<ElementPtr> expand(const LayoutContext& ctx) const;

private:
    QString m_ofProperty;
    int m_step = 1;
    ElementPtr m_template;
};

} // namespace BroadItem
