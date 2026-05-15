#include "broaditem/ContainerElement.h"
#include <QPainter>
#include <QDomElement>

namespace BroadItem {

ContainerElement::ContainerElement() = default;

void ContainerElement::parse(const QDomElement& xml)
{
    parseDecorators(xml);
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
    renderDecorators(painter, m_rect);
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
