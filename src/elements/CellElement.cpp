#include "broaditem/elements/CellElement.h"
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

const QSet<QString>& CellElement::supportedAttributes() const
{
    static const QSet<QString> attrs = {"v-align", "h-align"};
    return attrs;
}

void CellElement::parse(const QDomElement& xml)
{
    ContainerElement::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttribute("v-align"))
        m_vAlign = xml.attribute("v-align");
    if (xml.hasAttribute("h-align"))
        m_hAlign = xml.attribute("h-align");
}

ElementPtr CellElement::clone() const
{
    auto copy = std::make_shared<CellElement>();
    copy->decorators = decorators;
    copy->m_rect = m_rect;
    copy->m_vAlign = m_vAlign;
    copy->m_hAlign = m_hAlign;
    if (content())
        copy->setContent(content()->clone());
    return copy;
}

void CellElement::interpolateValues(const QStringList& values)
{
    if (content())
        content()->interpolateValues(values);
}

MeasureResult CellElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    return ContainerElement::measure(ctx, constraints);
}

void CellElement::layout(const LayoutContext& ctx, const Rect& rect)
{
    // Cell may be stretched by grid; we center/stretch content as specified
    ContainerElement::layout(ctx, rect);

    if (!content())
        return;

    // After ContainerElement::layout, content has been placed at top-left of content area.
    // We now adjust content position based on alignments if content is smaller than cell.
    // For simplicity, we re-layout content with adjusted rect.
    Rect contentArea;
    contentArea.pos.x = rect.pos.x + decorators.margin.left + decorators.border.width + decorators.padding.left;
    contentArea.pos.y = rect.pos.y + decorators.margin.top + decorators.border.width + decorators.padding.top;
    contentArea.size.width = std::max(0.0, rect.size.width - decorators.totalWidth());
    contentArea.size.height = std::max(0.0, rect.size.height - decorators.totalHeight());

    // Re-measure content to get intrinsic size
    auto result = content()->measure(ctx, LayoutConstraints{contentArea.size.width, contentArea.size.height});
    double cw = result.intrinsicSize.width;
    double ch = result.intrinsicSize.height;

    double cx = contentArea.pos.x;
    double cy = contentArea.pos.y;

    if (m_hAlign == "center")
        cx = contentArea.pos.x + (contentArea.size.width - cw) / 2.0;
    else if (m_hAlign == "right")
        cx = contentArea.pos.x + contentArea.size.width - cw;

    if (m_vAlign == "center")
        cy = contentArea.pos.y + (contentArea.size.height - ch) / 2.0;
    else if (m_vAlign == "bottom")
        cy = contentArea.pos.y + contentArea.size.height - ch;

    Rect childRect;
    childRect.pos.x = cx;
    childRect.pos.y = cy;
    childRect.size.width = std::min(cw, contentArea.size.width);
    childRect.size.height = std::min(ch, contentArea.size.height);

    content()->layout(ctx, childRect);
}

void CellElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    ContainerElement::render(painter, ctx);
}

bool CellElement::bindsProperty(const QString& name) const
{
    return ContainerElement::bindsProperty(name);
}

} // namespace BroadItem
