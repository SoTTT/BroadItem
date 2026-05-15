#pragma once

#include "../ContainerElement.h"

namespace BroadItem {

class CellElement : public ContainerElement {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const Rect& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    ElementPtr clone() const override;
    void interpolateValues(const QStringList& values) override;

private:
    QString m_vAlign = "center";
    QString m_hAlign = "center";
};

} // namespace BroadItem
