#include "broaditem/elements/ForElement.h"
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

void ForElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    if (xml.hasAttribute("bind"))
        m_bindProperty = xml.attribute("bind");
    if (xml.hasAttribute("step"))
        m_step = xml.attribute("step").toInt();
    if (m_step < 1)
        m_step = 1;
    if (xml.hasAttribute("space")) {
        m_space = parseDouble(xml.attribute("space"));
        m_hasExplicitSpace = true;
    }
}

void ForElement::setSpace(double space)
{
    m_space = space;
}

void ForElement::setBindProperty(const QString& bind)
{
    m_bindProperty = bind;
}

void ForElement::setTemplate(ElementPtr templ)
{
    m_template = std::move(templ);
}

ElementPtr ForElement::clone() const
{
    auto copy = std::make_shared<ForElement>();
    copy->m_bindProperty = m_bindProperty;
    copy->m_step = m_step;
    copy->m_space = m_space;
    copy->m_hasExplicitSpace = m_hasExplicitSpace;
    if (m_template)
        copy->m_template = m_template->clone();
    return copy;
}

std::vector<ElementPtr> ForElement::expand(const LayoutContext& ctx) const
{
    std::vector<ElementPtr> result;
    if (!m_template || m_bindProperty.isEmpty())
        return result;

    QVariant v = ctx.property(m_bindProperty);
    if (v.type() != QVariant::StringList) {
        if (v.isValid())
            qCritical() << "ForElement: bind property" << m_bindProperty
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

bool ForElement::bindsProperty(const QString& name) const
{
    return matchesProperty(m_bindProperty, name) || (m_template && m_template->bindsProperty(name));
}

} // namespace BroadItem
