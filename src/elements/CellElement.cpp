#include "broaditem/elements/CellElement.h"
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

const QSet<QString>& CellElement::supportedAttributes() const
{
    static const QSet<QString> attrs = QSet<QString>{"v-align", "h-align"} + boxModelAttributeNames();
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

void CellElement::interpolateValues(const QStringList& values)
{
    if (content())
        content()->interpolateValues(values);
}

MeasureResult CellElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    return ContainerElement::measure(ctx, constraints);
}

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

void CellElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    ContainerElement::render(painter, ctx);
}

bool CellElement::bindsProperty(const QString& name) const
{
    return ContainerElement::bindsProperty(name);
}

} // namespace BroadItem
