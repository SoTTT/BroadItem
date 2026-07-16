#include <broaditem/element/layout/CellElement.h>
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

/// @brief 返回 CellElement 支持的 XML 属性集合。
/// @return Reference to a static set containing "v-align", "h-align", and box model attributes.
const QSet<QString>& CellElement::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{"v-align", "h-align"} + boxModelAttributeNames();
    return attrs;
}

/// @brief 解析对齐设置的 XML 属性。
/// @param xml The DOM element to parse.
void CellElement::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttribute("v-align"))
        m_vAlign = xml.attribute("v-align");
    if (xml.hasAttribute("h-align"))
        m_hAlign = xml.attribute("h-align");
}

/// @brief 在单元格内布局子节点：多节点视为垂直堆叠块，块整体按对齐配置定位，
///        块内各子节点自上而下依次分配。
/// @param ctx The layout context.
/// @param rect The bounding rectangle assigned to this cell.
/// @param node 实例节点，rect 写入 node.rect。
void CellElement::layout(const LayoutContext& ctx, const QRectF& rect, Node& node) const
{
    node.rect = rect;
    if (node.children.empty())
        return;

    QRectF contentArea = contentRect(rect);
    LayoutConstraints childConstraints{contentArea.width(), contentArea.height()};

    // 测量堆叠块：总高为各子节点之和，宽为最大宽度。
    std::vector<QSizeF> childSizes;
    childSizes.reserve(node.children.size());
    double blockW = 0;
    double blockH = 0;
    for (const auto& child : node.children) {
        auto result = child->element->measure(ctx, childConstraints, *child);
        childSizes.push_back(result.intrinsicSize);
        blockW = std::max(blockW, result.intrinsicSize.width());
        blockH += result.intrinsicSize.height();
    }

    double cx = contentArea.x();
    double cy = contentArea.y();

    if (m_hAlign == "center")
        cx = contentArea.x() + (contentArea.width() - blockW) / 2.0;
    else if (m_hAlign == "right")
        cx = contentArea.x() + contentArea.width() - blockW;

    if (m_vAlign == "center")
        cy = contentArea.y() + (contentArea.height() - blockH) / 2.0;
    else if (m_vAlign == "bottom")
        cy = contentArea.y() + contentArea.height() - blockH;

    double y = cy;
    for (size_t i = 0; i < node.children.size(); ++i) {
        double cw = childSizes[i].width();
        double ch = childSizes[i].height();
        QRectF childRect(cx, y, std::min(cw, contentArea.width()), std::min(ch, contentArea.height()));
        node.children[i]->element->layout(ctx, childRect, *node.children[i]);
        y += ch;
    }
}

} // namespace BroadItem
