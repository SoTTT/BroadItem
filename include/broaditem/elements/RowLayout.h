#pragma once

#include "../ContainerElement.h"

namespace BroadItem {

class RowLayout : public ContainerElement {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    void addChild(ElementPtr child);
    double space() const { return m_space; }

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    ElementPtr clone() const override;
    void interpolateValues(const QStringList& values) override;

private:
    std::vector<ElementPtr> m_children;
    QString m_mainAlign = "start";
    QString m_crossAlign = "stretch";
    double m_space = 0;

    // Cached flattened children, populated during measure/layout and reused by render.
    mutable std::vector<ElementPtr> m_flattened;

    void layoutChildren(const LayoutContext& ctx, const QRectF& contentRect);

    std::vector<ElementPtr> flattenChildren(const LayoutContext& ctx) const;
};

} // namespace BroadItem
