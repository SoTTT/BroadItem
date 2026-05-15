#pragma once

#include "Element.h"

namespace BroadItem {

// A container wraps a content element with decorators (margin, border, background, padding)
class ContainerElement : public Element {
public:
    ContainerElement();

    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const Rect& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    void setContent(ElementPtr content);
    ElementPtr content() const { return m_content; }

    ElementPtr clone() const override;
    void interpolateValues(const QStringList& values) override;

    // Return all decorator pseudo-attribute names (margin*, padding*, border*, background*)
    static const QSet<QString>& decoratorAttributeNames();

private:
    ElementPtr m_content;

    void parseDecorators(const QDomElement& xml);
    void renderBackground(QPainter* p, const Rect& r) const;
    void renderBorder(QPainter* p, const Rect& r) const;
};

} // namespace BroadItem
