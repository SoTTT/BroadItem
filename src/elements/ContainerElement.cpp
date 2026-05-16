#include "broaditem/ContainerElement.h"
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

ContainerElement::ContainerElement() = default;

void ContainerElement::parse(const QDomElement& xml)
{
    parseBoxModel(xml);
}

MeasureResult ContainerElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    if (!m_content) {
        QSizeF sz(boxModelWidth(), boxModelHeight());
        return MeasureResult{sz};
    }

    LayoutConstraints childConstraints = constraints;
    double decoW = boxModelWidth();
    double decoH = boxModelHeight();

    if (constraints.availableWidth > 0)
        childConstraints.availableWidth = std::max(0.0, constraints.availableWidth - decoW);
    if (constraints.availableHeight > 0)
        childConstraints.availableHeight = std::max(0.0, constraints.availableHeight - decoH);

    auto result = m_content->measure(ctx, childConstraints);
    QSizeF sz(result.intrinsicSize.width() + decoW, result.intrinsicSize.height() + decoH);
    return MeasureResult{sz};
}

void ContainerElement::layout(const LayoutContext& ctx, const QRectF& rect)
{
    m_rect = rect;
    if (!m_content)
        return;

    QRectF cr = contentRect(rect);
    m_content->layout(ctx, cr);
}

void ContainerElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    renderBoxModel(painter, m_rect);
    if (m_content)
        m_content->render(painter, ctx);
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
    copy->m_margin = m_margin;
    copy->m_border = m_border;
    copy->m_background = m_background;
    copy->m_padding = m_padding;
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
