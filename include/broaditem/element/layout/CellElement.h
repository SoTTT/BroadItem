#pragma once

#include <broaditem/element/ContainerElement.h>

namespace BroadItem {

/// @brief 布局单元格元素，支持内容的垂直和水平对齐配置。
class CellElement : public ContainerElement {
public:
    void parse(const QDomElement& xml) override;
    void layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }

private:
    QString m_vAlign = "center";  ///< 内容的垂直对齐（"top"、"center"、"bottom"）。
    QString m_hAlign = "center";  ///< 内容的水平对齐（"left"、"center"、"right"）。
};

} // namespace BroadItem
