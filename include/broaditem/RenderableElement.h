#pragma once

#include "Element.h"
#include "BoxModel.h"

namespace BroadItem {

class RenderableElement : public Element {
public:
    virtual ~RenderableElement() = default;

    Margin  m_margin;
    Border  m_border;
    Background m_background;
    Padding m_padding;

    void parseBoxModel(const QDomElement& xml);
    void renderBoxModel(QPainter* painter, const QRectF& rect) const;
    QRectF contentRect(const QRectF& outerRect) const;

    double boxModelWidth() const {
        return m_margin.width() + m_border.width * 2 + m_padding.width();
    }
    double boxModelHeight() const {
        return m_margin.height() + m_border.width * 2 + m_padding.height();
    }

    static const QSet<QString>& boxModelAttributeNames();
};

} // namespace BroadItem
