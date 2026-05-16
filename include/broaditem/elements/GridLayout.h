#pragma once

#include "../ContainerElement.h"

namespace BroadItem {

class GridLayout : public ContainerElement {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    void addChild(ElementPtr child);

    int columns() const { return m_columns; }
    int rows() const { return m_rows; }
    double columnSpace() const { return m_columnSpace; }
    double rowSpace() const { return m_rowSpace; }

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    ElementPtr clone() const override;
    void interpolateValues(const QStringList& values) override;

private:
    std::vector<ElementPtr> m_children;
    int m_columns = 1;
    int m_rows = 1;
    double m_space = 0;
    double m_rowSpace = 0;
    double m_columnSpace = 0;

    struct CellMeasure {
        double width = 0;
        double height = 0;
    };
    std::vector<CellMeasure> m_cellMeasures;
    std::vector<double> m_colWidths;
    std::vector<double> m_rowHeights;

    // Cached flattened children, populated during measure/layout and reused by render.
    mutable std::vector<ElementPtr> m_flattened;

    std::vector<ElementPtr> flattenChildren(const LayoutContext& ctx) const;
};

} // namespace BroadItem
