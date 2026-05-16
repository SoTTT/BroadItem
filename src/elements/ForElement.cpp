#include "broaditem/elements/ForElement.h"
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

const QSet<QString>& ForElement::supportedAttributes() const
{
    static const QSet<QString> attrs = {":of", "step"};
    return attrs;
}

void ForElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttribute(":of"))
        m_ofProperty = xml.attribute(":of");
    if (xml.hasAttribute("step")) {
        if (validateInt(xml.attribute("step"), "step", m_step)) {
            if (m_step < 1) {
                qWarning() << "ForElement: step must be >= 1, got" << m_step;
                m_step = 1;
            }
        }
    }
}

void ForElement::setBindProperty(const QString& bind)
{
    m_ofProperty = bind;
}

void ForElement::setTemplate(ElementPtr templ)
{
    m_template = std::move(templ);
}

ElementPtr ForElement::clone() const
{
    auto copy = std::make_shared<ForElement>();
    copy->m_ofProperty = m_ofProperty;
    copy->m_step = m_step;
    if (m_template)
        copy->m_template = m_template->clone();
    return copy;
}

std::vector<ElementPtr> ForElement::expand(const LayoutContext& ctx) const
{
    std::vector<ElementPtr> result;
    if (!m_template || m_ofProperty.isEmpty())
        return result;

    QVariant v = ctx.property(m_ofProperty);
    // Type check: <for :of> requires QStringList for iteration.
    //   Pass — v.type() == QVariant::StringList; proceeds to clone the
    //          template and interpolate values per step.
    //   Fail — v is invalid (property unset → silently returns empty list,
    //          no output); or wrong type → qCritical with expected/got,
    //          returns empty list.
    if (v.type() != QVariant::StringList) {
        if (v.isValid())
            qCritical() << "ForElement: property" << m_ofProperty
                         << "expected QStringList, got" << v.typeName();
        return result;
    }

    QStringList values = v.toStringList();
    for (int i = 0; i < values.size(); i += m_step) {
        auto instance = m_template->clone();
        QStringList itemValues;
        for (int j = 0; j < m_step && (i + j) < values.size(); ++j) {
            itemValues.append(values[i + j]);
        }
        instance->interpolateValues(itemValues);
        result.push_back(instance);
    }
    return result;
}

MeasureResult ForElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    auto expanded = expand(ctx);
    if (expanded.empty())
        return MeasureResult{QSizeF(0, 0)};
    return expanded[0]->measure(ctx, constraints);
}

void ForElement::layout(const LayoutContext& ctx, const QRectF& rect)
{
    m_rect = rect;
    auto expanded = expand(ctx);
    if (!expanded.empty()) {
        expanded[0]->layout(ctx, rect);
    }
}

void ForElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    auto expanded = expand(ctx);
    if (!expanded.empty()) {
        expanded[0]->render(painter, ctx);
    }
}

bool ForElement::bindsProperty(const QString& name) const
{
    return matchesProperty(m_ofProperty, name) || (m_template && m_template->bindsProperty(name));
}

} // namespace BroadItem
