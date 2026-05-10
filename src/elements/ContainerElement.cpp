#include "broaditem/ContainerElement.h"
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

ContainerElement::ContainerElement()
{
    isControlElement = false;
}

void ContainerElement::parse(const QDomElement& xml)
{
    parseDecorators(xml);
}

void ContainerElement::parseDecorators(const QDomElement& xml)
{
    // Margin pseudo-properties
    if (xml.hasAttribute("margin"))
        decorators.margin.left = decorators.margin.right = decorators.margin.top = decorators.margin.bottom = parseDouble(xml.attribute("margin"));
    if (xml.hasAttribute("margin-left"))
        decorators.margin.left = parseDouble(xml.attribute("margin-left"));
    if (xml.hasAttribute("margin-right"))
        decorators.margin.right = parseDouble(xml.attribute("margin-right"));
    if (xml.hasAttribute("margin-top"))
        decorators.margin.top = parseDouble(xml.attribute("margin-top"));
    if (xml.hasAttribute("margin-bottom"))
        decorators.margin.bottom = parseDouble(xml.attribute("margin-bottom"));

    // Padding pseudo-properties
    if (xml.hasAttribute("padding"))
        decorators.padding.left = decorators.padding.right = decorators.padding.top = decorators.padding.bottom = parseDouble(xml.attribute("padding"));
    if (xml.hasAttribute("padding-left"))
        decorators.padding.left = parseDouble(xml.attribute("padding-left"));
    if (xml.hasAttribute("padding-right"))
        decorators.padding.right = parseDouble(xml.attribute("padding-right"));
    if (xml.hasAttribute("padding-top"))
        decorators.padding.top = parseDouble(xml.attribute("padding-top"));
    if (xml.hasAttribute("padding-bottom"))
        decorators.padding.bottom = parseDouble(xml.attribute("padding-bottom"));

    // Border pseudo-properties
    if (xml.hasAttribute("border-radius"))
        decorators.border.radius = parseDouble(xml.attribute("border-radius"));
    if (xml.hasAttribute("border-style"))
        decorators.border.style = xml.attribute("border-style");
    if (xml.hasAttribute("border-width"))
        decorators.border.width = parseDouble(xml.attribute("border-width"));
    if (xml.hasAttribute("border-color"))
        decorators.border.color = parseColor(xml.attribute("border-color"));

    // Background pseudo-properties
    if (xml.hasAttribute("background-color")) {
        decorators.background.color = parseColor(xml.attribute("background-color"));
        decorators.background.enabled = true;
    }
    if (xml.hasAttribute("background-radius")) {
        decorators.background.radius = parseDouble(xml.attribute("background-radius"));
        decorators.background.enabled = true;
    }
    if (xml.hasAttribute("background-transparent")) {
        decorators.background.transparent = parseDouble(xml.attribute("background-transparent"));
        decorators.background.enabled = true;
    }
}

MeasureResult ContainerElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    if (!m_content) {
        Size sz;
        sz.width = decorators.totalWidth();
        sz.height = decorators.totalHeight();
        return MeasureResult{sz};
    }

    LayoutConstraints childConstraints = constraints;
    double decoW = decorators.totalWidth();
    double decoH = decorators.totalHeight();

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    auto result = m_content->measure(ctx, childConstraints);
    Size sz;
    sz.width = result.intrinsicSize.width + decoW;
    sz.height = result.intrinsicSize.height + decoH;
    return MeasureResult{sz};
}

void ContainerElement::layout(const LayoutContext& ctx, const Rect& rect)
{
    m_rect = rect;
    if (!m_content)
        return;

    Rect contentRect;
    contentRect.pos.x = rect.pos.x + decorators.margin.left + decorators.border.width + decorators.padding.left;
    contentRect.pos.y = rect.pos.y + decorators.margin.top + decorators.border.width + decorators.padding.top;
    contentRect.size.width = std::max(0.0, rect.size.width - decorators.totalWidth());
    contentRect.size.height = std::max(0.0, rect.size.height - decorators.totalHeight());

    m_content->layout(ctx, contentRect);
}

void ContainerElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    // Compute decorator rectangles
    double mLeft = decorators.margin.left;
    double mTop = decorators.margin.top;
    double mRight = decorators.margin.right;
    double mBottom = decorators.margin.bottom;

    Rect marginRect;
    marginRect.pos.x = m_rect.pos.x;
    marginRect.pos.y = m_rect.pos.y;
    marginRect.size.width = m_rect.size.width;
    marginRect.size.height = m_rect.size.height;

    Rect borderRect;
    borderRect.pos.x = marginRect.pos.x + mLeft;
    borderRect.pos.y = marginRect.pos.y + mTop;
    borderRect.size.width = std::max(0.0, marginRect.size.width - mLeft - mRight);
    borderRect.size.height = std::max(0.0, marginRect.size.height - mTop - mBottom);

    // Background fills border-box area
    Rect bgRect = borderRect;

    renderBackground(painter, bgRect);
    renderBorder(painter, borderRect);
    if (m_content)
        m_content->render(painter, ctx);
}

void ContainerElement::renderBackground(QPainter* p, const Rect& r) const
{
    if (!decorators.background.visible())
        return;

    QColor c = decorators.background.color;
    c.setAlphaF(1.0 - decorators.background.transparent);
    p->setBrush(c);
    p->setPen(Qt::NoPen);

    double radius = decorators.background.radius;
    if (radius > 0)
        p->drawRoundedRect(r.toQRectF(), radius, radius);
    else
        p->drawRect(r.toQRectF());
}

void ContainerElement::renderBorder(QPainter* p, const Rect& r) const
{
    if (!decorators.border.visible())
        return;

    QPen pen(decorators.border.color);
    pen.setWidthF(decorators.border.width);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);

    double radius = decorators.border.radius;
    double halfW = decorators.border.width / 2.0;
    QRectF adjusted(r.pos.x + halfW, r.pos.y + halfW,
                    std::max(0.0, r.size.width - decorators.border.width),
                    std::max(0.0, r.size.height - decorators.border.width));

    if (radius > 0)
        p->drawRoundedRect(adjusted, radius, radius);
    else
        p->drawRect(adjusted);
}

bool ContainerElement::bindsProperty(const QString& name) const
{
    return m_content && m_content->bindsProperty(name);
}

void ContainerElement::setContent(ElementPtr content)
{
    m_content = std::move(content);
}

ElementPtr ContainerElement::clone() const
{
    auto copy = std::make_shared<ContainerElement>();
    copy->decorators = decorators;
    copy->m_rect = m_rect;
    if (m_content)
        copy->m_content = m_content->clone();
    return copy;
}

void ContainerElement::interpolateValues(const QStringList& values)
{
    if (m_content)
        m_content->interpolateValues(values);
}

} // namespace BroadItem
