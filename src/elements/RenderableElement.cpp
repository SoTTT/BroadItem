#include "broaditem/RenderableElement.h"
#include <QDomElement>
#include <QPainter>

namespace BroadItem {

const QSet<QString>& RenderableElement::decoratorAttributeNames()
{
    static const QSet<QString> attrs = {
        "margin", "margin-left", "margin-right", "margin-top", "margin-bottom",
        "padding", "padding-left", "padding-right", "padding-top", "padding-bottom",
        "border-radius", "border-style", "border-width", "border-color",
        "background-color", "background-radius", "background-transparent"
    };
    return attrs;
}

void RenderableElement::parseDecorators(const QDomElement& xml)
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

void RenderableElement::renderDecorators(QPainter* painter, const Rect& rect) const
{
    double mLeft = decorators.margin.left;
    double mTop = decorators.margin.top;
    double mRight = decorators.margin.right;
    double mBottom = decorators.margin.bottom;

    Rect borderRect;
    borderRect.pos.x = rect.pos.x + mLeft;
    borderRect.pos.y = rect.pos.y + mTop;
    borderRect.size.width = std::max(0.0, rect.size.width - mLeft - mRight);
    borderRect.size.height = std::max(0.0, rect.size.height - mTop - mBottom);

    // Background fills border-box area
    Rect bgRect = borderRect;

    // Render background
    if (decorators.background.visible()) {
        QColor c = decorators.background.color;
        c.setAlphaF(1.0 - decorators.background.transparent);
        painter->setBrush(c);
        painter->setPen(Qt::NoPen);

        double radius = decorators.background.radius;
        if (radius > 0)
            painter->drawRoundedRect(bgRect.toQRectF(), radius, radius);
        else
            painter->drawRect(bgRect.toQRectF());
    }

    // Render border
    if (decorators.border.visible()) {
        QPen pen(decorators.border.color);
        pen.setWidthF(decorators.border.width);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        double radius = decorators.border.radius;
        double halfW = decorators.border.width / 2.0;
        QRectF adjusted(borderRect.pos.x + halfW, borderRect.pos.y + halfW,
                        std::max(0.0, borderRect.size.width - decorators.border.width),
                        std::max(0.0, borderRect.size.height - decorators.border.width));

        if (radius > 0)
            painter->drawRoundedRect(adjusted, radius, radius);
        else
            painter->drawRect(adjusted);
    }
}

Rect RenderableElement::contentRect(const Rect& outerRect) const
{
    Rect content;
    content.pos.x = outerRect.pos.x + decorators.margin.left + decorators.border.width + decorators.padding.left;
    content.pos.y = outerRect.pos.y + decorators.margin.top + decorators.border.width + decorators.padding.top;
    content.size.width = std::max(0.0, outerRect.size.width - decorators.totalWidth());
    content.size.height = std::max(0.0, outerRect.size.height - decorators.totalHeight());
    return content;
}

} // namespace BroadItem
