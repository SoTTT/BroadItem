#pragma once

#include "RenderableElement.h"

namespace BroadItem {

// A container wraps a content element with decorators (margin, border, background, padding)
class ContainerElement : public RenderableElement {
public:
    ContainerElement();

    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    void setContent(ElementPtr content);
    ElementPtr content() const { return m_content; }

    ElementPtr clone() const override;
    void interpolateValues(const QStringList& values) override;

private:
    ElementPtr m_content;
};

} // namespace BroadItem
