#include "broaditem/elements/ForElement.h"
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

/// @brief 返回 ForElement 支持的 XML 属性集合。
/// @return Reference to a static set containing ":of" and "step".
const QSet<QString>& ForElement::supportedAttributes() const
{
    static const QSet<QString> attrs = {":of", "step"};
    return attrs;
}

/// @brief 从 XML 元素解析 :of 和 step 属性。
/// @param xml The DOM element to parse.
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

/// @brief 设置提供迭代列表的绑定属性。
/// @param bind The property name for the data source (QStringList).
void ForElement::setBindProperty(const QString& bind)
{
    m_ofProperty = bind;
}

/// @brief 设置要每次迭代克隆的模板元素。
/// @param templ The template element.
void ForElement::setTemplate(ElementPtr templ)
{
    m_template = std::move(templ);
}

/// @brief 创建此 ForElement 的深拷贝，包括模板。
/// @return A new ForElement with cloned template.
ElementPtr ForElement::clone() const
{
    auto copy = std::make_shared<ForElement>();
    copy->m_ofProperty = m_ofProperty;
    copy->m_step = m_step;
    if (m_template)
        copy->m_template = m_template->clone();
    return copy;
}

/// @brief 通过遍历绑定的 QStringList 将此控制元素展开为具体元素。
/// @param ctx The layout context providing the data property.
/// @return A vector of cloned and interpolated element instances.
std::vector<ElementPtr> ForElement::expand(const LayoutContext& ctx) const
{
    std::vector<ElementPtr> result;
    if (!m_template || m_ofProperty.isEmpty())
        return result;

    QVariant v = ctx.property(m_ofProperty);
    // Type check: <for :of> requires QStringList for iteration.
    //   Pass — v.userType() == QMetaType::QStringList; proceeds to clone the
    //          template and interpolate values per step.
    //   Fail — v is invalid (property unset → silently returns empty list,
    //          no output); or wrong type → qCritical with expected/got,
    //          returns empty list.
    if (v.userType() != QMetaType::QStringList) {
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

/// @brief 测量第一个展开的实例（控制元素将测量委托给展开的内容）。
/// @param ctx The layout context.
/// @param constraints Available width/height constraints.
/// @return The measured size of the first expanded element, or zero if empty.
MeasureResult ForElement::measure(const LayoutContext& ctx, const LayoutConstraints& constraints)
{
    auto expanded = expand(ctx);
    if (expanded.empty())
        return MeasureResult{QSizeF(0, 0)};
    return expanded[0]->measure(ctx, constraints);
}

/// @brief 在给定矩形内布局第一个展开的实例。
/// @param ctx The layout context.
/// @param rect The bounding rectangle for the first expanded element.
void ForElement::layout(const LayoutContext& ctx, const QRectF& rect)
{
    m_rect = rect;
    auto expanded = expand(ctx);
    if (!expanded.empty()) {
        expanded[0]->layout(ctx, rect);
    }
}

/// @brief 渲染第一个展开的实例。
/// @param painter The QPainter to render onto.
/// @param ctx The layout context.
void ForElement::render(QPainter* painter, const LayoutContext& ctx) const
{
    auto expanded = expand(ctx);
    if (!expanded.empty()) {
        expanded[0]->render(painter, ctx);
    }
}

/// @brief 检查此元素是否绑定指定属性（通过 :of 或在模板中）。
/// @param name The property name to check.
/// @return True if the property is bound.
bool ForElement::bindsProperty(const QString& name) const
{
    return matchesProperty(m_ofProperty, name) || (m_template && m_template->bindsProperty(name));
}

} // namespace BroadItem
