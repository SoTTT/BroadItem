#pragma once

#include "../ContainerElement.h"

namespace BroadItem {

/// @brief 布局单元格元素，支持内容的垂直和水平对齐配置。
class CellElement : public ContainerElement {
public:
    void parse(const QDomElement& xml) override;
    MeasureResult measure(const LayoutContext& ctx, const LayoutConstraints& constraints) override;
    void layout(const LayoutContext& ctx, const QRectF& rect) override;
    void render(QPainter* painter, const LayoutContext& ctx) const override;
    bool bindsProperty(const QString& name) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

    ElementPtr clone() const override;
    void interpolateValues(const QStringList& values) override;

private:
    QString m_vAlign = "center";  ///< Vertical alignment of content ("top", "center", "bottom").
    QString m_hAlign = "center";  ///< Horizontal alignment of content ("left", "center", "right").
};

} // namespace BroadItem
