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

void RenderableElement::renderDecorators(QPainter* painter, const QRectF& rect) const
{
    double mLeft = decorators.margin.left;
    double mTop = decorators.margin.top;
    double mRight = decorators.margin.right;
    double mBottom = decorators.margin.bottom;

    QRectF borderRect(rect.x() + mLeft, rect.y() + mTop,
                      std::max(0.0, rect.width() - mLeft - mRight),
                      std::max(0.0, rect.height() - mTop - mBottom));

    // Background fills border-box area
    QRectF bgRect = borderRect;

    // Render background
    if (decorators.background.visible()) {
        QColor c = decorators.background.color;
        c.setAlphaF(1.0 - decorators.background.transparent);
        painter->setBrush(c);
        painter->setPen(Qt::NoPen);

        double radius = decorators.background.radius;
        if (radius > 0)
            painter->drawRoundedRect(bgRect, radius, radius);
        else
            painter->drawRect(bgRect);
    }

    // Render border
    if (decorators.border.visible()) {
        QPen pen(decorators.border.color);
        pen.setWidthF(decorators.border.width);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        double radius = decorators.border.radius;
        double halfW = decorators.border.width / 2.0;
        QRectF adjusted(borderRect.x() + halfW, borderRect.y() + halfW,
                        std::max(0.0, borderRect.width() - decorators.border.width),
                        std::max(0.0, borderRect.height() - decorators.border.width));

        if (radius > 0)
            painter->drawRoundedRect(adjusted, radius, radius);
        else
            painter->drawRect(adjusted);
    }
}

QRectF RenderableElement::contentRect(const QRectF& outerRect) const
{
    return QRectF(outerRect.x() + decorators.margin.left + decorators.border.width + decorators.padding.left,
                  outerRect.y() + decorators.margin.top + decorators.border.width + decorators.padding.top,
                  std::max(0.0, outerRect.width() - decorators.totalWidth()),
                  std::max(0.0, outerRect.height() - decorators.totalHeight()));
}

} // namespace BroadItem
