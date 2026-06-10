#include "broaditem/elements/ForElement.h"
#include "broaditem/MapPropertyContext.h"
#include "broaditem/ItemPropertyContext.h"
#include <QDomElement>
#include <QDebug>

namespace BroadItem {

/// @brief 返回 ForElement 支持的 XML 属性集合。
/// @return Reference to a static set containing ":of" and ":as".
const QSet<QString>& ForElement::supportedAttributes() const
{
    static const QSet<QString> attrs = {":of", ":as"};
    return attrs;
}

/// @brief 从 XML 元素解析 :of 和 :as 属性。
/// @param xml The DOM element to parse.
void ForElement::parse(const QDomElement& xml)
{
    Element::parse(xml);
    validateAttributes(xml);
    if (xml.hasAttribute(":of"))
        m_ofProperty = xml.attribute(":of");
    QString asVal = xml.attribute(":as");
    if (!asVal.isEmpty())
        m_asVariable = asVal;
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
    copy->m_asVariable = m_asVariable;
    if (m_template)
        copy->m_template = m_template->clone();
    return copy;
}

/// @brief 通过遍历绑定的列表将此控制元素展开为具体元素。
///
/// 支持 :of 为 QStringList 或 QVariantList。为每个迭代项创建 per-item
/// MapPropertyContext（含层次和平铺键），通过 ItemPropertyContext 链到全局 context，
/// 克隆模板并调用 resolveBindings() 解析每个克隆的属性绑定。
///
/// @param ctx The layout context providing the data property.
/// @return A vector of cloned and resolved element instances.
std::vector<ElementPtr> ForElement::expand(const LayoutContext& ctx) const
{
    std::vector<ElementPtr> result;
    if (!m_template || m_ofProperty.isEmpty())
        return result;

    if (m_asVariable.isEmpty()) {
        qCritical() << "ForElement: :as attribute is required, but not set";
        return result;
    }

    QVariant v = ctx.property(m_ofProperty);
    if (!v.isValid())
        return result;

    if (v.userType() == QMetaType::QStringList) {
        QStringList values = v.toStringList();
        for (const auto& val : values) {
            auto itemCtx = std::make_shared<MapPropertyContext>();
            itemCtx->setProperty(m_asVariable, val);
            ItemPropertyContext chainedCtx(itemCtx.get(), ctx.ctx, m_asVariable);
            LayoutContext itemLayoutCtx{&chainedCtx};

            auto instance = m_template->clone();
            instance->resolveBindings(itemLayoutCtx);
            result.push_back(instance);
        }
    } else if (v.userType() == QMetaType::QVariantList) {
        QVariantList list = v.toList();
        for (const auto& val : list) {
            if (!val.isValid()) {
                qWarning() << "ForElement: skipping null item in list";
                continue;
            }
            auto itemCtx = std::make_shared<MapPropertyContext>();
            itemCtx->setProperty(m_asVariable, val);
            if (val.userType() == QMetaType::QVariantMap) {
                QVariantMap map = val.toMap();
                for (auto it = map.begin(); it != map.end(); ++it)
                    itemCtx->setProperty(it.key(), it.value());
            }
            ItemPropertyContext chainedCtx(itemCtx.get(), ctx.ctx, m_asVariable);
            LayoutContext itemLayoutCtx{&chainedCtx};

            auto instance = m_template->clone();
            instance->resolveBindings(itemLayoutCtx);
            result.push_back(instance);
        }
    } else {
        qCritical() << "ForElement: property" << m_ofProperty
                     << "expected QStringList or QVariantList, got" << v.typeName();
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
    // 1. Check if the :of data source property changed → always triggers relayout
    if (matchesProperty(m_ofProperty, name))
        return true;

    // 2. Check template bindings, but strip :as prefix — per-item variables
    //    are not global properties and should not trigger relayout.
    if (m_template) {
        if (!m_asVariable.isEmpty()) {
            // The :as variable itself (e.g. "item") and per-item paths
            // (e.g. "item.name") are NOT global — do not trigger relayout
            QString prefix = m_asVariable + ".";
            if (name == m_asVariable || name.startsWith(prefix))
                return false;
        }
        return m_template->bindsProperty(name);
    }

    return false;
}

} // namespace BroadItem
