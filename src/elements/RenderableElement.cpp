#include "broaditem/RenderableElement.h"
#include <QDomElement>
#include <QPainter>

namespace BroadItem {

const QSet<QString>& RenderableElement::boxModelAttributeNames()
{
    static const QSet<QString> attrs = {
        "margin", "margin-left", "margin-right", "margin-top", "margin-bottom",
        "padding", "padding-left", "padding-right", "padding-top", "padding-bottom",
        "border-radius", "border-style", "border-width", "border-color",
        "background-color", "background-radius", "background-opacity"
    };
    return attrs;
}

void RenderableElement::parseBoxModel(const QDomElement& xml)
{
    if (xml.hasAttribute("margin"))
        m_margin.left = m_margin.right = m_margin.top = m_margin.bottom = parseDouble(xml.attribute("margin"));
    if (xml.hasAttribute("margin-left"))
        m_margin.left = parseDouble(xml.attribute("margin-left"));
    if (xml.hasAttribute("margin-right"))
        m_margin.right = parseDouble(xml.attribute("margin-right"));
    if (xml.hasAttribute("margin-top"))
        m_margin.top = parseDouble(xml.attribute("margin-top"));
    if (xml.hasAttribute("margin-bottom"))
        m_margin.bottom = parseDouble(xml.attribute("margin-bottom"));

    if (xml.hasAttribute("padding"))
        m_padding.left = m_padding.right = m_padding.top = m_padding.bottom = parseDouble(xml.attribute("padding"));
    if (xml.hasAttribute("padding-left"))
        m_padding.left = parseDouble(xml.attribute("padding-left"));
    if (xml.hasAttribute("padding-right"))
        m_padding.right = parseDouble(xml.attribute("padding-right"));
    if (xml.hasAttribute("padding-top"))
        m_padding.top = parseDouble(xml.attribute("padding-top"));
    if (xml.hasAttribute("padding-bottom"))
        m_padding.bottom = parseDouble(xml.attribute("padding-bottom"));

    if (xml.hasAttribute("border-radius"))
        m_border.radius = parseDouble(xml.attribute("border-radius"));
    if (xml.hasAttribute("border-style"))
        m_border.style = xml.attribute("border-style");
    if (xml.hasAttribute("border-width"))
        m_border.width = parseDouble(xml.attribute("border-width"));
    if (xml.hasAttribute("border-color"))
        m_border.color = parseColor(xml.attribute("border-color"));

    if (xml.hasAttribute("background-color")) {
        m_background.color = parseColor(xml.attribute("background-color"));
        m_background.enabled = true;
    }
    if (xml.hasAttribute("background-radius")) {
        m_background.radius = parseDouble(xml.attribute("background-radius"));
        m_background.enabled = true;
    }
    if (xml.hasAttribute("background-opacity")) {
        m_background.opacity = parseDouble(xml.attribute("background-opacity"));
        m_background.enabled = true;
    }
}

void RenderableElement::renderBoxModel(QPainter* painter, const QRectF& rect) const
{
    double mLeft = m_margin.left;
    double mTop = m_margin.top;
    double mRight = m_margin.right;
    double mBottom = m_margin.bottom;

    QRectF borderRect(rect.x() + mLeft, rect.y() + mTop,
                      std::max(0.0, rect.width() - mLeft - mRight),
                      std::max(0.0, rect.height() - mTop - mBottom));

    QRectF bgRect = borderRect;

    if (m_background.visible()) {
        QColor c = m_background.color;
        c.setAlphaF(m_background.opacity);
        painter->setBrush(c);
        painter->setPen(Qt::NoPen);

        double radius = m_background.radius;
        if (radius > 0)
            painter->drawRoundedRect(bgRect, radius, radius);
        else
            painter->drawRect(bgRect);
    }

    if (m_border.visible()) {
        QPen pen(m_border.color);
        pen.setWidthF(m_border.width);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        double radius = m_border.radius;
        double halfW = m_border.width / 2.0;
        QRectF adjusted(borderRect.x() + halfW, borderRect.y() + halfW,
                        std::max(0.0, borderRect.width() - m_border.width),
                        std::max(0.0, borderRect.height() - m_border.width));

        if (radius > 0)
            painter->drawRoundedRect(adjusted, radius, radius);
        else
            painter->drawRect(adjusted);
    }
}

QRectF RenderableElement::contentRect(const QRectF& outerRect) const
{
    return QRectF(outerRect.x() + m_margin.left + m_border.width + m_padding.left,
                  outerRect.y() + m_margin.top + m_border.width + m_padding.top,
                  std::max(0.0, outerRect.width() - boxModelWidth()),
                  std::max(0.0, outerRect.height() - boxModelHeight()));
}

} // namespace BroadItem
