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

/// @brief 创建此单元格元素的深拷贝。
/// @return A new CellElement with copied properties and cloned content.
ElementPtr CellElement::clone() const
{
    auto copy = std::make_shared<CellElement>();
    copy->m_margin = m_margin;
    copy->m_border = m_border;
    copy->m_background = m_background;
    copy->m_padding = m_padding;
    copy->m_rect = m_rect;
    copy->m_vAlign = m_vAlign;
    copy->m_hAlign = m_hAlign;
    if (content())
        copy->setContent(content()->clone());
    return copy;
}

/// @brief 通过委托 ContainerElement::measure() 测量单元格。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @return The measured size of the cell.
MeasureResult CellElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    return ContainerElement::measure(ctx, constraints);
}

/// @brief 在单元格内布局内容元素，应用水平和垂直对齐。
/// @param ctx The layout context.
/// @param rect The bounding rectangle assigned to this cell.
void CellElement::layout(const LayoutContext& ctx, const QRectF& rect)
{
    ContainerElement::layout(ctx, rect);

    if (!content())
        return;

    QRectF contentArea = contentRect(rect);

    auto result = content()->measure(ctx, LayoutConstraints{contentArea.width(), contentArea.height()});
    double cw = result.intrinsicSize.width();
    double ch = result.intrinsicSize.height();

    double cx = contentArea.x();
    double cy = contentArea.y();

    if (m_hAlign == "center")
        cx = contentArea.x() + (contentArea.width() - cw) / 2.0;
    else if (m_hAlign == "right")
        cx = contentArea.x() + contentArea.width() - cw;

    if (m_vAlign == "center")
        cy = contentArea.y() + (contentArea.height() - ch) / 2.0;
    else if (m_vAlign == "bottom")
        cy = contentArea.y() + contentArea.height() - ch;

    QRectF childRect(cx, cy, std::min(cw, contentArea.width()), std::min(ch, contentArea.height()));
    content()->layout(ctx, childRect);
}

/// @brief 通过 ContainerElement::render() 渲染单元格及其内容。
/// @param painter The QPainter to render onto.
/// @param ctx The layout context.
void CellElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    ContainerElement::render(painter, ctx);
}

/// @brief 检查此单元格或其内容是否绑定指定属性。
/// @param name The property name to check.
/// @return True if the property is bound.
bool CellElement::bindsProperty(const QString& name) const
{
    return ContainerElement::bindsProperty(name);
}

} // namespace BroadItem
