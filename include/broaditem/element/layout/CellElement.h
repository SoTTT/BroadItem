#pragma once

#include <broaditem/element/ContainerElement.h>
#include <broaditem/element/Alignment.h>

namespace BroadItem {

/// @brief 布局单元格元素，支持内容的垂直和水平对齐配置。
class CellElement : public ContainerElement {
public:
    void parse(const QDomElement& xml) override;
    void layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const override;

    const QSet<QString>& supportedAttributes() const override;
    bool canHaveChildren() const override { return true; }
    /// @brief 解析期挂载：保留第一个子元素作为内容，多余忽略。
    void addParsedChild(const ElementPtr& child) override;

private:
    VAlign m_vAlign = VAlign::Center;  ///< 内容的垂直对齐。
    HAlign m_hAlign = HAlign::Center;  ///< 内容的水平对齐。
};

} // namespace BroadItem
